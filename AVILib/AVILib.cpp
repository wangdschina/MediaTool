// AVILib.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

/* The following variable indicates the kind of error */

long AVI_errno = 0;

/*******************************************************************
 *                                                                 *
 *    Utilities for writing an AVI File                            *
 *                                                                 *
 *******************************************************************/

/* AVI_MAX_LEN: The maximum length of an AVI file, we stay a bit below
    the 2GB limit (Remember: 2*10^9 is smaller than 2 GB) */

#define AVI_MAX_LEN 2000000000

/* HEADERBYTES: The number of bytes to reserve for the header */

#define HEADERBYTES 2048

#define PAD_EVEN(x) ( ((x)+1) & ~1 )


/* Copy n into dst as a 4 byte, little endian number.
   Should also work on big endian machines */

static void long2str(unsigned char *dst, int n)
{
   dst[0] = (n    )&0xff;
   dst[1] = (n>> 8)&0xff;
   dst[2] = (n>>16)&0xff;
   dst[3] = (n>>24)&0xff;
}

/* Convert a string of 4 or 2 bytes to a number,
   also working on big endian machines */

static unsigned long str2ulong(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) | (str[2]<<16) | (str[3]<<24) );
}
static unsigned long str2ushort(unsigned char *str)
{
   return ( str[0] | (str[1]<<8) );
}

/* Calculate audio sample size from number of bits and number of channels.
   This may have to be adjusted for eg. 12 bits and stereo */

static int avi_sampsize(avi_t *AVI)
{
   int s;
   s = ((AVI->a_bits+7)/8)*AVI->a_chans;
   if(s==0) s=1; /* avoid possible zero divisions */
   return s;
}


static int avi_add_index_entry(avi_t *AVI, unsigned char *tag, long flags, long pos, long len)
{
   void *ptr;

   if(AVI->n_idx>=AVI->max_idx)
   {
      ptr = realloc((void *)AVI->idx,(AVI->max_idx+4096)*16);
      if(ptr == 0)
      {
         AVI_errno = AVI_ERR_NO_MEM;
         return -1;
      }
      AVI->max_idx += 4096;
      AVI->idx = (unsigned char((*)[16]) ) ptr;
   }

   /* Add index entry */

   memcpy(AVI->idx[AVI->n_idx],tag,4);
   long2str(AVI->idx[AVI->n_idx]+ 4,flags);
   long2str(AVI->idx[AVI->n_idx]+ 8,pos);
   long2str(AVI->idx[AVI->n_idx]+12,len);

   /* Update counter */

   AVI->n_idx++;

   return 0;
}

#define OUT4CC(s) \
   if(nhb<=HEADERBYTES-4) memcpy(AVI_header+nhb,s,4); nhb += 4

#define OUTLONG(n) \
   if(nhb<=HEADERBYTES-4) long2str(AVI_header+nhb,n); nhb += 4

#define OUTSHRT(n) \
   if(nhb<=HEADERBYTES-2) { \
      AVI_header[nhb  ] = (n   )&0xff; \
      AVI_header[nhb+1] = (n>>8)&0xff; \
   } \
   nhb += 2


/*******************************************************************
 *                                                                 *
 *    Utilities for reading video and audio from an AVI File       *
 *                                                                 *
 *******************************************************************/

int AVI_close(avi_t *AVI)
{
   /* Even if there happened a error, we first clean up */

	if (AVI->fdes)
	{
		fclose(AVI->fdes);
		AVI->fdes = NULL;
	}
	if(AVI->idx) 
	{
		   free(AVI->idx);
		   AVI->idx = NULL;
	}
	if(AVI->video_index) 
	{
		free(AVI->video_index);
		AVI->video_index = NULL;
	}
	if(AVI->audio_index) 
	{
		free(AVI->audio_index);
		AVI->audio_index = NULL;
	}
	if (AVI)
	{
		free(AVI);
		AVI = NULL;
	}

	return 0;
}


#define ERR_EXIT(x) \
{ \
   AVI_close(AVI); \
   AVI_errno = x; \
   return 0; \
}

avi_t *AVI_open_input_file(const char *filename, int getIndex)
{
   avi_t *AVI;
   long i, n, rate, scale, idx_type;
   unsigned char *hdrl_data;
   long hdrl_len = 0;
   long nvi, nai, ioff;
   long tot;
   int lasttag = 0;
   int vids_strh_seen = 0;
   int vids_strf_seen = 0;
   int auds_strh_seen = 0;
   int auds_strf_seen = 0;
   int num_stream = 0;
   char data[256];

   /* Create avi_t structure */

   AVI = (avi_t *) malloc(sizeof(avi_t));
   if(AVI==NULL)
   {
      AVI_errno = AVI_ERR_NO_MEM;
      return 0;
   }
   memset((void *)AVI,0,sizeof(avi_t));

   AVI->mode = AVI_MODE_READ; /* open for reading */

   /* Open the file */
	
   AVI->fdes = fopen(filename, "rb");
   if(AVI->fdes == nullptr)
   {
      AVI_errno = AVI_ERR_OPEN;
      free(AVI);
      return 0;
   }

   /* Read first 12 bytes and check that this is an AVI file */

   if( fread(data, sizeof(char), 12, AVI->fdes) != 12 ) ERR_EXIT(AVI_ERR_READ)

   if( memcmp(data  ,"RIFF",4) !=0 ||
       memcmp(data+8,"AVI ",4) !=0 ) ERR_EXIT(AVI_ERR_NO_AVI)

   /* Go through the AVI file and extract the header list,
      the start position of the 'movi' list and an optionally
      present idx1 tag */

   hdrl_data = 0;

   while(1)
   {
      if( fread(data, sizeof(char), 8, AVI->fdes) != 8 ) break; /* We assume it's EOF */

      n = str2ulong((unsigned char*)(data+4));
      n = PAD_EVEN(n);

      if(memcmp(data,"LIST",4) == 0)
      {
         if( fread(data, sizeof(char), 4, AVI->fdes) != 4 ) ERR_EXIT(AVI_ERR_READ)
         n -= 4;
         if(memcmp(data,"hdrl",4) == 0)
         {
            hdrl_len = n;
            hdrl_data = (unsigned char *) malloc(n);
            if(hdrl_data==0) ERR_EXIT(AVI_ERR_NO_MEM)
            if( fread(hdrl_data, 1, n, AVI->fdes) != n ) ERR_EXIT(AVI_ERR_READ)
         }
         else if(memcmp(data,"movi",4) == 0)
         {
            //AVI->movi_start = fseek(AVI->fdes,0,SEEK_CUR);
			 AVI->movi_start = ftell(AVI->fdes);
            fseek(AVI->fdes,n,SEEK_CUR);
         }
         else
            fseek(AVI->fdes,n,SEEK_CUR);
      }
      else if(memcmp(data,"idx1",4) == 0)
      {
         /* n must be a multiple of 16, but the reading does not
            break if this is not the case */

         AVI->n_idx = AVI->max_idx = n/16;
         AVI->idx = (unsigned  char((*)[16]) ) malloc(n);
         if(AVI->idx==0) ERR_EXIT(AVI_ERR_NO_MEM)
         if( fread(AVI->idx, 1, n, AVI->fdes) != n ) ERR_EXIT(AVI_ERR_READ)
      }
      else
         fseek(AVI->fdes,n,SEEK_CUR);
   }

   if(!hdrl_data      ) ERR_EXIT(AVI_ERR_NO_HDRL)
   if(!AVI->movi_start) ERR_EXIT(AVI_ERR_NO_MOVI)

   /* Interpret the header list */

   for(i=0;i<hdrl_len;)
   {
      /* List tags are completly ignored */

      if(memcmp(hdrl_data+i,"LIST",4)==0) { i+= 12; continue; }

      n = str2ulong(hdrl_data+i+4);
      n = PAD_EVEN(n);

      /* Interpret the tag and its args */

      if(memcmp(hdrl_data+i,"strh",4)==0)
      {
         i += 8;
         if(memcmp(hdrl_data+i,"vids",4) == 0 && !vids_strh_seen)
         {
            memcpy(AVI->compressor,hdrl_data+i+4,4);
            AVI->compressor[4] = 0;
            scale = str2ulong(hdrl_data+i+20);
            rate  = str2ulong(hdrl_data+i+24);
            if(scale!=0) AVI->fps = (double)rate/(double)scale;
            AVI->video_frames = str2ulong(hdrl_data+i+32);
            AVI->video_strn = num_stream;
            vids_strh_seen = 1;
            lasttag = 1; /* vids */
         }
         else if (memcmp (hdrl_data+i,"auds",4) ==0 && ! auds_strh_seen)
         {
            AVI->audio_bytes = str2ulong(hdrl_data+i+32)*avi_sampsize(AVI);
            AVI->audio_strn = num_stream;
            auds_strh_seen = 1;
            lasttag = 2; /* auds */
         }
         else
            lasttag = 0;
         num_stream++;
      }
      else if(memcmp(hdrl_data+i,"strf",4)==0)
      {
         i += 8;
         if(lasttag == 1)
         {
            AVI->width  = str2ulong(hdrl_data+i+4);
            AVI->height = str2ulong(hdrl_data+i+8);
            vids_strf_seen = 1;
         }
         else if(lasttag == 2)
         {
            AVI->a_fmt   = str2ushort(hdrl_data+i  );
            AVI->a_chans = str2ushort(hdrl_data+i+2);
            AVI->a_rate  = str2ulong (hdrl_data+i+4);
            AVI->a_bits  = str2ushort(hdrl_data+i+14);
            auds_strf_seen = 1;
         }
         lasttag = 0;
      }
      else
      {
         i += 8;
         lasttag = 0;
      }

      i += n;
   }

   free(hdrl_data);

   if(!vids_strh_seen || !vids_strf_seen || AVI->video_frames==0) ERR_EXIT(AVI_ERR_NO_VIDS)

   AVI->video_tag[0] = AVI->video_strn/10 + '0';
   AVI->video_tag[1] = AVI->video_strn%10 + '0';
   AVI->video_tag[2] = 'd';
   AVI->video_tag[3] = 'b';

   /* Audio tag is set to "99wb" if no audio present */
   if(!AVI->a_chans) AVI->audio_strn = 99;

   AVI->audio_tag[0] = AVI->audio_strn/10 + '0';
   AVI->audio_tag[1] = AVI->audio_strn%10 + '0';
   AVI->audio_tag[2] = 'w';
   AVI->audio_tag[3] = 'b';

   fseek(AVI->fdes,AVI->movi_start,SEEK_SET);

   /* get index if wanted */

   if(!getIndex) return AVI;

   /* if the file has an idx1, check if this is relative
      to the start of the file or to the start of the movi list */

   idx_type = 0;

   if(AVI->idx)
   {
      long pos;
		unsigned long len;

      /* Search the first videoframe in the idx1 and look where
         it is in the file */

      for(i=0;i<AVI->n_idx;i++)
         if( memcmp(AVI->idx[i],AVI->video_tag,3)==0 ) break;
      if(i>=AVI->n_idx) ERR_EXIT(AVI_ERR_NO_VIDS)

      pos = str2ulong(AVI->idx[i]+ 8);
      len = str2ulong(AVI->idx[i]+12);

      fseek(AVI->fdes,pos,SEEK_SET);
      if(fread(data, sizeof(char), 8, AVI->fdes)!=8) ERR_EXIT(AVI_ERR_READ)
      if( memcmp(data,AVI->idx[i],4)==0 && str2ulong((unsigned char*)(data+4))==len )
      {
         idx_type = 1; /* Index from start of file */
      }
      else
      {
         fseek(AVI->fdes,pos+AVI->movi_start-4,SEEK_SET);
         if(fread(data, sizeof(char), 8, AVI->fdes)!=8) ERR_EXIT(AVI_ERR_READ)
         if( memcmp(data,AVI->idx[i],4)==0 && str2ulong((unsigned char*)(data+4))==len )
         {
            idx_type = 2; /* Index from start of movi list */
         }
      }
      /* idx_type remains 0 if neither of the two tests above succeeds */
   }

   if(idx_type == 0)
   {
      /* we must search through the file to get the index */

      fseek(AVI->fdes, AVI->movi_start, SEEK_SET);

      AVI->n_idx = 0;

      while(1)
      {
         if( fread(data, sizeof(char), 8, AVI->fdes) != 8 ) break;
         n = str2ulong((unsigned char*)data+4);

         /* The movi list may contain sub-lists, ignore them */

         if(memcmp(data,"LIST",4)==0)
         {
            fseek(AVI->fdes,4,SEEK_CUR);
            continue;
         }

         /* Check if we got a tag ##db, ##dc or ##wb */

         if( ( (data[2]=='d' || data[2]=='D') &&
               (data[3]=='b' || data[3]=='B' || data[3]=='c' || data[3]=='C') )
          || ( (data[2]=='w' || data[2]=='W') &&
               (data[3]=='b' || data[3]=='B') ) )
         {
            avi_add_index_entry(AVI,(unsigned char*)data,0,fseek(AVI->fdes,0,SEEK_CUR)-8,n);
         }

         fseek(AVI->fdes,PAD_EVEN(n),SEEK_CUR);
      }
      idx_type = 1;
   }

   /* Now generate the video index and audio index arrays */

   nvi = 0;
   nai = 0;

   for(i=0;i<AVI->n_idx;i++)
   {
      if(memcmp(AVI->idx[i],AVI->video_tag,3) == 0) nvi++;
      if(memcmp(AVI->idx[i],AVI->audio_tag,4) == 0) nai++;
   }

   AVI->video_frames = nvi;
   AVI->audio_chunks = nai;

   if(AVI->video_frames==0) ERR_EXIT(AVI_ERR_NO_VIDS)
   AVI->video_index = (video_index_entry *) malloc(nvi*sizeof(video_index_entry));
   if(AVI->video_index==0) ERR_EXIT(AVI_ERR_NO_MEM)
   if(AVI->audio_chunks)
   {
      AVI->audio_index = (audio_index_entry *) malloc(nai*sizeof(audio_index_entry));
      if(AVI->audio_index==0) ERR_EXIT(AVI_ERR_NO_MEM)
   }

   nvi = 0;
   nai = 0;
   tot = 0;
   ioff = idx_type == 1 ? 8 : AVI->movi_start+4;

   for(i=0;i<AVI->n_idx;i++)
   {
      if(memcmp(AVI->idx[i],AVI->video_tag,3) == 0)
      {
         AVI->video_index[nvi].pos = str2ulong(AVI->idx[i]+ 8)+ioff;
         AVI->video_index[nvi].len = str2ulong(AVI->idx[i]+12);
         nvi++;
      }
      if(memcmp(AVI->idx[i],AVI->audio_tag,4) == 0)
      {
         AVI->audio_index[nai].pos = str2ulong(AVI->idx[i]+ 8)+ioff;
         AVI->audio_index[nai].len = str2ulong(AVI->idx[i]+12);
         AVI->audio_index[nai].tot = tot;
         tot += AVI->audio_index[nai].len;
         nai++;
      }
   }

   AVI->audio_bytes = tot;

   /* Reposition the file */

   fseek(AVI->fdes,AVI->movi_start,SEEK_SET);
   AVI->video_pos = 0;

   return AVI;
}

long AVI_video_frames(avi_t *AVI)
{
   return AVI->video_frames;
}
int  AVI_video_width(avi_t *AVI)
{
   return AVI->width;
}
int  AVI_video_height(avi_t *AVI)
{
   return AVI->height;
}
double AVI_video_frame_rate(avi_t *AVI)
{
   return AVI->fps;
}
char* AVI_video_compressor(avi_t *AVI)
{
   return AVI->compressor;
}

long AVI_frame_size(avi_t *AVI, long frame)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(frame < 0 || frame >= AVI->video_frames) return 0;
   return(AVI->video_index[frame].len);
}

int AVI_seek_start(avi_t *AVI)
{
   //fseek(AVI->fdes,AVI->movi_start,SEEK_SET);
   fseek(AVI->fdes, AVI->movi_start, SEEK_SET);
   AVI->video_pos = 0;
   AVI->audio_posc = 0;
   AVI->audio_posb = 0;
   return 0;
}

int AVI_set_video_position(avi_t *AVI, long frame, long *frame_len)
{
   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if (frame < 0 ) frame = 0;
   AVI->video_pos = frame;
   if (frame_len != NULL)
     *frame_len = AVI->video_index[frame].len;
   return 0;
}
      

long AVI_read_frame(avi_t *AVI, char *vidbuf)
{
   long n;

   if(AVI->mode==AVI_MODE_WRITE) { AVI_errno = AVI_ERR_NOT_PERM; return -1; }
   if(!AVI->video_index)         { AVI_errno = AVI_ERR_NO_IDX;   return -1; }

   if(AVI->video_pos < 0 || AVI->video_pos >= AVI->video_frames) return 0;
   n = AVI->video_index[AVI->video_pos].len;

   fseek(AVI->fdes, AVI->video_index[AVI->video_pos].pos, SEEK_SET);
   if (fread(vidbuf, 1, n, AVI->fdes) != n)
   {
      AVI_errno = AVI_ERR_READ;
      return -1;
   }

   AVI->video_pos++;

   return n;
}

/* AVI_read_data: Special routine for reading the next audio or video chunk
                  without having an index of the file. */

int AVI_read_data(avi_t *AVI, char *vidbuf, long max_vidbuf,
                              char *audbuf, long max_audbuf,
                              long *len)
{

/*
 * Return codes:
 *
 *    1 = video data read
 *    2 = audio data read
 *    0 = reached EOF
 *   -1 = video buffer too small
 *   -2 = audio buffer too small
 */

   int n;
   char data[8];

   if(AVI->mode==AVI_MODE_WRITE) return 0;

   while(1)
   {
      /* Read tag and length */

      if( fread(data,sizeof(char), 8, AVI->fdes) != 8 ) return 0;

      /* if we got a list tag, ignore it */

      if(memcmp(data,"LIST",4) == 0)
      {
         fseek(AVI->fdes,4,SEEK_CUR);
         continue;
      }

      n = PAD_EVEN(str2ulong((unsigned char*)data+4));

      if(memcmp(data,AVI->video_tag,3) == 0)
      {
         *len = n;
         AVI->video_pos++;
         if(n>max_vidbuf)
         {
            fseek(AVI->fdes,n,SEEK_CUR);
            return -1;
         }
         if(fread(vidbuf, 1, n, AVI->fdes) != n ) return 0;
         return 1;
      }
      else if(memcmp(data,AVI->audio_tag,4) == 0)
      {
         *len = n;
         if(n>max_audbuf)
         {
            fseek(AVI->fdes,n,SEEK_CUR);
            return -2;
         }
         if(fread(audbuf, 1, n, AVI->fdes) != n ) return 0;
         return 2;
         break;
      }
      else
         if(fseek(AVI->fdes,n,SEEK_CUR)<0)  return 0;
   }
}

/* AVI_print_error: Print most recent error (similar to perror) */

char *(avi_errors[]) =
{
  /*  0 */ "avilib - No Error",
  /*  1 */ "avilib - AVI file size limit reached",
  /*  2 */ "avilib - Error opening AVI file",
  /*  3 */ "avilib - Error reading from AVI file",
  /*  4 */ "avilib - Error writing to AVI file",
  /*  5 */ "avilib - Error writing index (file may still be useable)",
  /*  6 */ "avilib - Error closing AVI file",
  /*  7 */ "avilib - Operation (read/write) not permitted",
  /*  8 */ "avilib - Out of memory (malloc failed)",
  /*  9 */ "avilib - Not an AVI file",
  /* 10 */ "avilib - AVI file has no header list (corrupted?)",
  /* 11 */ "avilib - AVI file has no MOVI list (corrupted?)",
  /* 12 */ "avilib - AVI file has no video data",
  /* 13 */ "avilib - operation needs an index",
  /* 14 */ "avilib - Unkown Error"
};
static int num_avi_errors = sizeof(avi_errors)/sizeof(char*);

static char error_string[4096];

