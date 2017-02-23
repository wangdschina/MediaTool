#pragma once

#include "tinyxml.h"
#include "GameAccount.h"
#include "TianHongCardOrder.h"
#include "RechargeOrder.h"
#include "TypeDef.h"

//////////////////////////////////宏//////////////////////////////////
#define TXS_VERSION _T("1.0")
#define SUCCEED_CODE 0
#define UNKNOWN_ERROR_CODE 1
#define ADMINISTRATOR_ROLE_NAME  _T("管理员")

// 0~255
enum NET_PACKIT_TYPES
{
	ID_PACKIT_HEARTBEAT,				// 心跳包
	ID_PACKIT_HEARTBEAT_ACK,			// 心跳包应答
	ID_PACKIT_XMLSTREAM,				// 按Xml流解析
	ID_PACKIT_BYTESTREAM				// 按字节流解析
};

// 协议Xml流格式
/*
<?xml version="1.0" encoding="gb2312" ?>
	<TXS> --协议根节点
		<Version>1.0</Version> --协议版本
		<Service>User_Login</Service> --协议功能
		<Data> --协议相关数据
			<UserName>T1</UserName>
		</Data>
	</TXS>
</xml>
*/

/*
<?xml version="1.0" encoding="gb2312" ?>
	<TXS> --协议根节点
		<Version>1.0</Version> --协议版本
		<Service>Client_Login_Ack</Service> --协议功能
		<Data> --协议相关数据
			<Result>0<Result> // 0表示成功 其他表示失败
		</Data>
	</TXS>
</xml>
*/

class CAssistUser
{
public:
	CAssistUser() 
	{
		m_nId = 0;
		m_strUserName = _T("");
		m_strRoleName = _T("");
		m_strLoginTime = _T("");
		m_strAddress = _T("");
		m_nFinishOrderCnt = 0;

		m_nRechargeOrderIdUse = 0;
		m_nGameAccountIdUse = 0;
	}

	~CAssistUser()
	{
	}

public:
	int m_nId;										// 用户ID
	tstring m_strUserName;							// 用户名
	tstring m_strRoleName;							// 角色名
	tstring m_strLoginTime;							// 登录上线时间
	tstring m_strAddress;							// 内网地址
	int m_nFinishOrderCnt;							// 完成订单数

	// 当前使用的订单和游戏帐号
	int m_nRechargeOrderIdUse;						// 使用充值订单ID
	int m_nGameAccountIdUse;						// 使用游戏帐号ID
};


class CAssistProtocol
{
public:
	static void InsertTextToEndChild(TiXmlElement& tiXmlElement, LPCTSTR lpszFmt, ...)
	{
		va_list vMarker; 
		va_start(vMarker, lpszFmt);
		int nLen = _vsntprintf(NULL, 0, lpszFmt, vMarker) + 1;
		tstring strText;
		std::vector<TCHAR> vBuffer(nLen, '\0');
		_vsntprintf(&vBuffer[0], vBuffer.size(), lpszFmt, vMarker);
		strText.assign(&vBuffer[0]);
		va_end(vMarker);

		TiXmlText tiXmlText(CStringUtil::TStrToUtf8(strText).c_str());
		tiXmlElement.InsertEndChild(tiXmlText);
	}

	static bool GetProtoBsService(tstring& strService, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;

		return true;
	}

	static std::string BuildProtoXmlString(const tstring& strService, const TiXmlElement& dataElement)
	{
		// 创建XML的文档对象
		TiXmlDocument document;

		// 创建声明节点并连接 
		TiXmlDeclaration xmlDecl("1.0", "utf-8", "");
		document.InsertEndChild(xmlDecl);

		// 创建根元素节点
		TiXmlElement rootElement("TXS");
		// 创建Ver元素节点
		TiXmlElement versionElement("Version");
		// 创建ServiceE元素节点
		TiXmlElement serviceElement("Service");

		tstring strVersion = TXS_VERSION;
		InsertTextToEndChild(versionElement, _T("%s"), strVersion.c_str());
		InsertTextToEndChild(serviceElement, _T("%s"), strService.c_str());
		rootElement.InsertEndChild(versionElement);
		rootElement.InsertEndChild(serviceElement);
		rootElement.InsertEndChild(dataElement);
		document.InsertEndChild(rootElement);

		TiXmlPrinter printer; 
		printer.SetIndent(""); // 设置缩进为空便于网络传输
		printer.SetLineBreak(""); // 设置换行为空便于网络传输
		document.Accept(&printer); 
		return printer.CStr();
	}

	static bool ParseProtoXmlString(const std::string& strXml, tstring& strService, TiXmlElement& dataElement)
	{
		// 解析时不压缩空格
		TiXmlBase::SetCondenseWhiteSpace(false);

		// 创建XML的文档对象
		TiXmlDocument document;
		document.Parse(strXml.c_str());

		TiXmlElement* pRootElement = document.RootElement();
		if (pRootElement == NULL)
		{
			return false;
		}

		TiXmlElement* pVersionElement = pRootElement->FirstChildElement("Version");
		if (pVersionElement == NULL)
		{
			return false;
		}
		tstring strVersion = CStringUtil::Utf8ToTStr(pVersionElement->GetText());
		if (strVersion != TXS_VERSION)
		{
			return false;
		}

		TiXmlElement* pServiceElement = pRootElement->FirstChildElement("Service");
		if (pServiceElement == NULL)
		{
			return false;
		}
		strService = CStringUtil::Utf8ToTStr(pServiceElement->GetText());

		TiXmlElement* pDataElement = pRootElement->FirstChildElement("Data");
		if (pDataElement == NULL)
		{ 
			return false;
		}
		dataElement = *pDataElement;

		return true;
	}

	static void BuildXmlBs(const std::string& strXml, CByteStream& outByteStream)
	{
		outByteStream.Write((unsigned char)ID_PACKIT_XMLSTREAM);
		outByteStream.Write((unsigned char*)strXml.c_str(), strXml.length());
	}

	static bool ParseXmlBs(std::string& strXml, CByteStream& inByteStream)
	{
		inByteStream.SetReadOffset(0);
		unsigned char packetType;
		if (!inByteStream.Read(packetType))
			return false;

		if (packetType != ID_PACKIT_XMLSTREAM)
			return false;

		char* pszXml = new char[inByteStream.GetNumberOfBytesUsed()];
		memset(pszXml, 0, inByteStream.GetNumberOfBytesUsed());
		if (!inByteStream.Read((unsigned char*)pszXml, inByteStream.GetNumberOfBytesUsed()-1))
		{
			delete [] pszXml;
			return false;
		}

		strXml = pszXml;
		delete [] pszXml;
		return true;
	}

	//////////////////////////////////////////客户端发送&&相应解析///////////////////////////////////////////
	static void BuildCheckUpdateBs(const tstring& strVersion, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement versionElement("Version");

		InsertTextToEndChild(versionElement, _T("%s"), strVersion.c_str());
		dataElement.InsertEndChild(versionElement);

		BuildXmlBs(BuildProtoXmlString(_T("Check_Update"), dataElement), outByteStream);
	}

	static bool ParseCheckUpdateBs(tstring& strVersion, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Check_Update"))
			return false;

		TiXmlElement* pVersionElement = dataElement.FirstChildElement("Version");
		if (pVersionElement == NULL)
			return false;

		if (pVersionElement->GetText() != NULL)
			strVersion = CStringUtil::Utf8ToTStr(pVersionElement->GetText());

		return true;
	}

	static void BuildUserLoginBs(const tstring& strUserName, const tstring& strPassword, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement userNameElement("UserName");
		TiXmlElement passwordElement("Password");

		InsertTextToEndChild(userNameElement, _T("%s"), strUserName.c_str());
		InsertTextToEndChild(passwordElement, _T("%s"), strPassword.c_str());
		dataElement.InsertEndChild(userNameElement);
		dataElement.InsertEndChild(passwordElement);

		BuildXmlBs(BuildProtoXmlString(_T("User_Login"), dataElement), outByteStream);
	}

	static bool ParseUserLoginBs(tstring& strUserName, tstring& strPassword, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("User_Login"))
			return false;

		TiXmlElement* pUserNameElement = dataElement.FirstChildElement("UserName");
		if (pUserNameElement == NULL)
			return false;

		TiXmlElement* pPasswordElement = dataElement.FirstChildElement("Password");
		if (pPasswordElement == NULL)
			return false;

		if (pUserNameElement->GetText() != NULL)
			strUserName = CStringUtil::Utf8ToTStr(pUserNameElement->GetText());

		if (pPasswordElement->GetText() != NULL)
			strPassword = CStringUtil::Utf8ToTStr(pPasswordElement->GetText());

		return true;
	}

	static void BuildSelectRechargeOrderUseBs(int nOrderId, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement orderIdElement("OrderId");

		InsertTextToEndChild(orderIdElement, _T("%d"), nOrderId);
		dataElement.InsertEndChild(orderIdElement);

		BuildXmlBs(BuildProtoXmlString(_T("Select_RechargeOrder_Use"), dataElement), outByteStream);
	}

	static bool ParseSelectRechargeOrderUseBs(int& nOrderId, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Select_RechargeOrder_Use"))
			return false;

		TiXmlElement* pOrderIdElement = dataElement.FirstChildElement("OrderId");
		if (pOrderIdElement == NULL)
			return false;

		if (pOrderIdElement->GetText() != NULL)
			nOrderId = CStringUtil::ToInt(CStringUtil::Utf8ToTStr(pOrderIdElement->GetText()));

		return true;
	}

	static void BuildGetGameAccountBs(int nChannelId, const tstring& strGameName, int nOffset, int nCount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement channelIdElement("ChannelId");
		TiXmlElement gameNameElement("GameName");
		TiXmlElement offsetElement("Offset");
		TiXmlElement countElement("Count");

		InsertTextToEndChild(channelIdElement, _T("%d"), nChannelId);
		InsertTextToEndChild(gameNameElement, _T("%s"), strGameName.c_str());
		InsertTextToEndChild(offsetElement, _T("%d"), nOffset);
		InsertTextToEndChild(countElement, _T("%d"), nCount);
		dataElement.InsertEndChild(channelIdElement);
		dataElement.InsertEndChild(gameNameElement);
		dataElement.InsertEndChild(offsetElement);
		dataElement.InsertEndChild(countElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_GameAccount"), dataElement), outByteStream);
	}

	static bool ParseGetGameAccountBs(int& nChannelId, tstring& strGameName, int& nOffset, int& nCount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_GameAccount"))
			return false;

		TiXmlElement* pChannelIdElement = dataElement.FirstChildElement("ChannelId");
		if (pChannelIdElement == NULL)
			return false;

		TiXmlElement* pGameNameElement = dataElement.FirstChildElement("GameName");
		if (pGameNameElement == NULL)
			return false;

		TiXmlElement* pOffsetElement = dataElement.FirstChildElement("Offset");
		if (pOffsetElement == NULL)
			return false;

		TiXmlElement* pCountElement = dataElement.FirstChildElement("Count");
		if (pCountElement == NULL)
			return false;

		if (pChannelIdElement->GetText() != NULL)
			nChannelId = CStringUtil::ToInt(CStringUtil::Utf8ToTStr(pChannelIdElement->GetText()));

		if (pGameNameElement->GetText() != NULL)
			strGameName = CStringUtil::Utf8ToTStr(pGameNameElement->GetText());

		if (pOffsetElement->GetText() != NULL)
			nOffset = CStringUtil::ToInt(CStringUtil::Utf8ToTStr(pOffsetElement->GetText()));

		if (pCountElement->GetText() != NULL)
			nCount = CStringUtil::ToInt(CStringUtil::Utf8ToTStr(pCountElement->GetText()));

		return true;
	}

	static void BuildSelectGameAccountUseBs(int nGameAccountId, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement gameAccountIdElement("GameAccountId");

		InsertTextToEndChild(gameAccountIdElement, _T("%d"), nGameAccountId);
		dataElement.InsertEndChild(gameAccountIdElement);

		BuildXmlBs(BuildProtoXmlString(_T("Select_GameAccount_Use"), dataElement), outByteStream);
	}

	static bool ParseSelectGameAccountUseBs(int& nGameAccountId, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Select_GameAccount_Use"))
			return false;

		TiXmlElement* pGameAccountIdElement = dataElement.FirstChildElement("GameAccountId");
		if (pGameAccountIdElement == NULL)
			return false;

		if (pGameAccountIdElement->GetText() != NULL)
			nGameAccountId = CStringUtil::ToInt(CStringUtil::Utf8ToTStr(pGameAccountIdElement->GetText()));

		return true;
	}

	static void BuildAddRechargePayOrderBs(const CRechargeOrder& rechargeOrder, const std::vector<CPayOrder>& vecPayOrder, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement idElement("Id");
		TiXmlElement typeElement("Type");
		TiXmlElement numberElement("Number");
		TiXmlElement customerIdElement("CustomerId");
		TiXmlElement productNameElement("ProductName");
		TiXmlElement priceElement("Price");
		TiXmlElement payPriceElement("PayPrice");
		TiXmlElement qQNumberElement("QQNumber");
		TiXmlElement telePhoneNumberElement("TelephoneNumber");
		TiXmlElement roleNameElement("RoleName");
		TiXmlElement standbyRoleNameElement("StandbyRoleName");
		TiXmlElement roleAttriElement("RoleAttri");
		TiXmlElement gameAccountIdElement("GameAccountId");
		TiXmlElement userNameElement("UserName");
		TiXmlElement passwordElement("Password");
		TiXmlElement gameNameElement("GameName");
		TiXmlElement serviceAreaElement("ServiceArea");
		TiXmlElement channelElement("Channel");
		TiXmlElement stateElement("State");
		TiXmlElement buyTimeElement("BuyTime");
		TiXmlElement payChannelElment("PayChannel");
		TiXmlElement payChannelOrderNumberElement("PayChannelOrderNumber");
		TiXmlElement bankOrderNumberElement("BankOrderNumber");
		TiXmlElement payerElement("Payer");
		TiXmlElement deliveryNoteElement("DeliveryNote");
		TiXmlElement payNoteElement("PayNote");
		TiXmlElement gifeCodeElement("GifeCode");
		TiXmlElement rebateRateElement("RebateRatio");

		TiXmlElement snapshotListElement("SnapshotList");
		for (int i = 0; i < rechargeOrder.m_vecSnapshot.size(); i++)
		{
			TiXmlElement snapshotElement("Snapshot");
			snapshotElement.SetAttribute("type", rechargeOrder.m_vecSnapshot[i].m_nType);
			snapshotElement.SetAttribute("hashFileName", CStringUtil::TStrToUtf8(rechargeOrder.m_vecSnapshot[i].m_strHashFileName).c_str());
			snapshotElement.SetAttribute("originalFileName", CStringUtil::TStrToUtf8(rechargeOrder.m_vecSnapshot[i].m_strOriginalFileName).c_str());
			snapshotElement.SetAttribute("originalFilePath", CStringUtil::TStrToUtf8(rechargeOrder.m_vecSnapshot[i].m_strOriginalFilePath).c_str());

			snapshotListElement.InsertEndChild(snapshotElement);
		}

		TiXmlElement payOrderListElement("PayOrderList");
		for (int i = 0; i < vecPayOrder.size(); i++)
		{
			TiXmlElement payOrderElement("PayOrder");
			payOrderElement.SetAttribute("channelId", vecPayOrder[i].m_nChannelId);
			payOrderElement.SetAttribute("orderId", CStringUtil::TStrToUtf8(vecPayOrder[i].m_nOrderId).c_str());
			payOrderElement.SetAttribute("loginAccount", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strLoginAccount).c_str());
			payOrderElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strUserName).c_str());
			payOrderElement.SetAttribute("gameName", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strGameName).c_str());
			payOrderElement.SetAttribute("buyAccount", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strBuyAccount).c_str());
			payOrderElement.SetAttribute("buyPassword", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strBuyPassword).c_str());
			payOrderElement.SetAttribute("productName", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strProductName).c_str());
			payOrderElement.SetAttribute("productType", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strProductType).c_str());
			payOrderElement.SetDoubleAttribute("buyPrice", vecPayOrder[i].m_fBuyPrice);
			payOrderElement.SetAttribute("buyQuantity", vecPayOrder[i].m_nBuyQuantity);
			payOrderElement.SetAttribute("salePrice", vecPayOrder[i].m_nSalePrice);
			payOrderElement.SetDoubleAttribute("totalAmount", vecPayOrder[i].m_fTotalAmount);
			payOrderElement.SetDoubleAttribute("realAmount", vecPayOrder[i].m_fRealPrice);
			payOrderElement.SetDoubleAttribute("profit", vecPayOrder[i].m_fProfit);
			payOrderElement.SetAttribute("buyTime", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strBuyTime).c_str());
			payOrderElement.SetAttribute("oprIP", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strOprIP).c_str());
			payOrderElement.SetAttribute("expirTime", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strExpirationTime).c_str());
			payOrderElement.SetAttribute("remarks", CStringUtil::TStrToUtf8(vecPayOrder[i].m_strRemarks).c_str());

			payOrderListElement.InsertEndChild(payOrderElement);
		}

		InsertTextToEndChild(idElement, _T("%d"), rechargeOrder.m_nId);
		InsertTextToEndChild(typeElement, _T("%d"), rechargeOrder.m_nType);
		InsertTextToEndChild(numberElement, _T("%s"), rechargeOrder.m_strNumber.c_str());
		InsertTextToEndChild(customerIdElement, _T("%d"), rechargeOrder.m_nCustomerId);
		InsertTextToEndChild(productNameElement, _T("%s"), rechargeOrder.m_strProductName.c_str());
		InsertTextToEndChild(priceElement, _T("%f"), rechargeOrder.m_fPrice);
		InsertTextToEndChild(payPriceElement, _T("%f"), rechargeOrder.m_fPayPrice);
		InsertTextToEndChild(qQNumberElement, _T("%s"), rechargeOrder.m_strQQNumber.c_str());
		InsertTextToEndChild(telePhoneNumberElement, _T("%s"), rechargeOrder.m_strTelephoneNumber.c_str());
		InsertTextToEndChild(roleNameElement, _T("%s"), rechargeOrder.m_strRoleName.c_str());
		InsertTextToEndChild(standbyRoleNameElement, _T("%s"), rechargeOrder.m_strStandbyRoleName.c_str());
		InsertTextToEndChild(roleAttriElement, _T("%s"), rechargeOrder.m_strRoleAttri.c_str());
		InsertTextToEndChild(gameAccountIdElement, _T("%d"), rechargeOrder.m_nGameAccountId);
		InsertTextToEndChild(userNameElement, _T("%s"), rechargeOrder.m_strUserName.c_str());
		InsertTextToEndChild(passwordElement, _T("%s"), rechargeOrder.m_strPassword.c_str());
		InsertTextToEndChild(gameNameElement, _T("%s"), rechargeOrder.m_strGameName.c_str());
		InsertTextToEndChild(serviceAreaElement, _T("%s"), rechargeOrder.m_strServiceArea.c_str());
		InsertTextToEndChild(channelElement, _T("%s"), rechargeOrder.m_strChannel.c_str());
		InsertTextToEndChild(stateElement, _T("%s"), rechargeOrder.m_strState.c_str());
		InsertTextToEndChild(buyTimeElement, _T("%s"), rechargeOrder.m_strBuyTime.c_str());
		InsertTextToEndChild(payChannelElment, _T("%s"), rechargeOrder.m_strPayChannel.c_str());
		InsertTextToEndChild(payChannelOrderNumberElement, _T("%s"), rechargeOrder.m_strPayChannelOrderNumber.c_str());
		InsertTextToEndChild(bankOrderNumberElement, _T("%s"), rechargeOrder.m_strBankOrderNumber.c_str());
		InsertTextToEndChild(payerElement, _T("%s"), rechargeOrder.m_strPayer.c_str());
		InsertTextToEndChild(deliveryNoteElement, _T("%s"), rechargeOrder.m_strDeliveryNote.c_str());
		InsertTextToEndChild(payNoteElement, _T("%s"), rechargeOrder.m_strPayNote.c_str());
		InsertTextToEndChild(gifeCodeElement, _T("%s"), rechargeOrder.m_strGifeCode.c_str());
		InsertTextToEndChild(rebateRateElement, _T("%f"), rechargeOrder.m_fRebateRatio);

		dataElement.InsertEndChild(idElement);
		dataElement.InsertEndChild(typeElement);
		dataElement.InsertEndChild(numberElement);
		dataElement.InsertEndChild(customerIdElement);
		dataElement.InsertEndChild(productNameElement);
		dataElement.InsertEndChild(priceElement);
		dataElement.InsertEndChild(payPriceElement);
		dataElement.InsertEndChild(qQNumberElement);
		dataElement.InsertEndChild(telePhoneNumberElement);
		dataElement.InsertEndChild(roleNameElement);
		dataElement.InsertEndChild(standbyRoleNameElement);
		dataElement.InsertEndChild(roleAttriElement);
		dataElement.InsertEndChild(gameAccountIdElement);
		dataElement.InsertEndChild(userNameElement);
		dataElement.InsertEndChild(passwordElement);
		dataElement.InsertEndChild(gameNameElement);
		dataElement.InsertEndChild(serviceAreaElement);
		dataElement.InsertEndChild(channelElement);
		dataElement.InsertEndChild(stateElement);
		dataElement.InsertEndChild(buyTimeElement);
		dataElement.InsertEndChild(payChannelElment);
		dataElement.InsertEndChild(payChannelOrderNumberElement);
		dataElement.InsertEndChild(bankOrderNumberElement);
		dataElement.InsertEndChild(payerElement);
		dataElement.InsertEndChild(deliveryNoteElement);
		dataElement.InsertEndChild(payNoteElement);
		dataElement.InsertEndChild(gifeCodeElement);
		dataElement.InsertEndChild(rebateRateElement);
		dataElement.InsertEndChild(snapshotListElement);
		dataElement.InsertEndChild(payOrderListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_RechargePayOrder"), dataElement), outByteStream);
	}

	static bool ParseAddRechargePayOrderBs(CRechargeOrder& rechargeOrder, std::vector<CPayOrder>& vecPayOrder, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_RechargePayOrder"))
			return false;

		TiXmlElement* pIdElement = dataElement.FirstChildElement("Id");
		if (pIdElement == NULL)
			return false;

		TiXmlElement* pTypeElement = dataElement.FirstChildElement("Type");
		if (pTypeElement == NULL)
			return false;

		TiXmlElement* pNumberElement = dataElement.FirstChildElement("Number");
		if (pNumberElement == NULL)
			return false;

		TiXmlElement* pCustomeIdElement = dataElement.FirstChildElement("CustomerId");
		if (pCustomeIdElement == NULL)
			return false;

		TiXmlElement* pProductNameElement = dataElement.FirstChildElement("ProductName");
		if (pProductNameElement == NULL)
			return false;

		TiXmlElement* pPriceElement = dataElement.FirstChildElement("Price");
		if (pPriceElement == NULL)
			return false;

		TiXmlElement* pPayPriceElement = dataElement.FirstChildElement("PayPrice");
		if (pPayPriceElement == NULL)
			return false;

		TiXmlElement* pQQNumberElement = dataElement.FirstChildElement("QQNumber");
		if (pQQNumberElement == NULL)
			return false;

		TiXmlElement* pTelephoneNumberElement = dataElement.FirstChildElement("TelephoneNumber");
		if (pTelephoneNumberElement == NULL)
			return false;

		TiXmlElement* pRoleNameElement = dataElement.FirstChildElement("RoleName");
		if (pRoleNameElement == NULL)
			return false;

		TiXmlElement* pStandbyRoleNameElement = dataElement.FirstChildElement("StandbyRoleName");
		if (pStandbyRoleNameElement == NULL)
			return false;

		TiXmlElement* pRoleAttriElement = dataElement.FirstChildElement("RoleAttri");
		if (pRoleAttriElement == NULL)
			return false;

		TiXmlElement* pGameAccountIdElement = dataElement.FirstChildElement("GameAccountId");
		if (pGameAccountIdElement == NULL)
			return false;

		TiXmlElement* pUserNameElement = dataElement.FirstChildElement("UserName");
		if (pUserNameElement == NULL)
			return false;

		TiXmlElement* pPasswordElement = dataElement.FirstChildElement("Password");
		if (pPasswordElement == NULL)
			return false;

		TiXmlElement* pServiceAreaElement = dataElement.FirstChildElement("ServiceArea");
		if (pServiceAreaElement == NULL)
			return false;

		TiXmlElement* pGameNameElement = dataElement.FirstChildElement("GameName");
		if (pGameNameElement == NULL)
			return false;

		TiXmlElement* pChannelElement = dataElement.FirstChildElement("Channel");
		if (pChannelElement == NULL)
			return false;

		TiXmlElement* pStateElement = dataElement.FirstChildElement("State");
		if (pStateElement == NULL)
			return false;

		TiXmlElement* pBuyTimeElement = dataElement.FirstChildElement("BuyTime");
		if (pBuyTimeElement == NULL)
			return false;

		TiXmlElement* pPayChannelElement = dataElement.FirstChildElement("PayChannel");
		if (pPayChannelElement == NULL)
			return false;

		TiXmlElement* pPayChannelOrderNumberElement = dataElement.FirstChildElement("PayChannelOrderNumber");
		if (pPayChannelOrderNumberElement == NULL)
			return false;

		TiXmlElement* pBankOrderNumberElement = dataElement.FirstChildElement("BankOrderNumber");
		if (pBankOrderNumberElement == NULL)
			return false;

		TiXmlElement* pPayerElement = dataElement.FirstChildElement("Payer");
		if (pPayerElement == NULL)
			return false;

		TiXmlElement* pDeliveryNoteElement = dataElement.FirstChildElement("DeliveryNote");
		if (pDeliveryNoteElement == NULL)
			return false;

		TiXmlElement* pPayNoteElement = dataElement.FirstChildElement("PayNote");
		if (pPayNoteElement == NULL)
			return false;

		TiXmlElement* pGifeCodeElement = dataElement.FirstChildElement("GifeCode");
		if (pGifeCodeElement == NULL)
			return false;

		TiXmlElement* pRebateRatioElement = dataElement.FirstChildElement("RebateRatio");
		if (pRebateRatioElement == NULL)
			return false;

		TiXmlElement* pSnapshotListElement = dataElement.FirstChildElement("SnapshotList");
		if (pSnapshotListElement == NULL)
			return false;

		TiXmlElement* pPayOrderListElement = dataElement.FirstChildElement("PayOrderList");
		if (pPayOrderListElement == NULL)
			return false;

		if (pIdElement->GetText() != NULL)
			rechargeOrder.m_nId = atoi(pIdElement->GetText());

		if (pTypeElement->GetText() != NULL)
			rechargeOrder.m_nType = atoi(pTypeElement->GetText());

		if (pNumberElement->GetText() != NULL)
			rechargeOrder.m_strNumber = CStringUtil::Utf8ToTStr(pNumberElement->GetText());

		if (pCustomeIdElement->GetText() != NULL)
			rechargeOrder.m_nCustomerId = atoi(pCustomeIdElement->GetText());

		if (pProductNameElement->GetText() != NULL)
			rechargeOrder.m_strProductName = CStringUtil::Utf8ToTStr(pProductNameElement->GetText());

		if (pPriceElement->GetText() != NULL)
			rechargeOrder.m_fPrice = atof(pPriceElement->GetText());

		if (pPayPriceElement->GetText() != NULL)
			rechargeOrder.m_fPayPrice = atof(pPayPriceElement->GetText());

		if (pQQNumberElement->GetText() != NULL)
			rechargeOrder.m_strQQNumber = CStringUtil::Utf8ToTStr(pQQNumberElement->GetText());

		if (pTelephoneNumberElement->GetText() != NULL)
			rechargeOrder.m_strTelephoneNumber = CStringUtil::Utf8ToTStr(pTelephoneNumberElement->GetText());

		if (pRoleNameElement->GetText() != NULL)
			rechargeOrder.m_strRoleName = CStringUtil::Utf8ToTStr(pRoleNameElement->GetText());

		if (pStandbyRoleNameElement->GetText() != NULL)
			rechargeOrder.m_strStandbyRoleName = CStringUtil::Utf8ToTStr(pStandbyRoleNameElement->GetText());

		if (pRoleAttriElement->GetText() != NULL)
			rechargeOrder.m_strRoleAttri = CStringUtil::Utf8ToTStr(pRoleAttriElement->GetText());

		if (pGameAccountIdElement->GetText() != NULL)
			rechargeOrder.m_nGameAccountId = atoi(pGameAccountIdElement->GetText());

		if (pUserNameElement->GetText() != NULL)
			rechargeOrder.m_strUserName = CStringUtil::Utf8ToTStr(pUserNameElement->GetText());

		if (pPasswordElement->GetText() != NULL)
			rechargeOrder.m_strPassword = CStringUtil::Utf8ToTStr(pPasswordElement->GetText());

		if (pGameNameElement->GetText() != NULL)
			rechargeOrder.m_strGameName = CStringUtil::Utf8ToTStr(pGameNameElement->GetText());

		if (pServiceAreaElement->GetText() != NULL)
			rechargeOrder.m_strServiceArea = CStringUtil::Utf8ToTStr(pServiceAreaElement->GetText());

		if (pChannelElement->GetText() != NULL)
			rechargeOrder.m_strChannel = CStringUtil::Utf8ToTStr(pChannelElement->GetText());

		if (pStateElement->GetText() != NULL)
			rechargeOrder.m_strState = CStringUtil::Utf8ToTStr(pStateElement->GetText());

		if (pBuyTimeElement->GetText() != NULL)
			rechargeOrder.m_strBuyTime = CStringUtil::Utf8ToTStr(pBuyTimeElement->GetText());

		if (pPayChannelElement->GetText() != NULL)
			rechargeOrder.m_strPayChannel = CStringUtil::Utf8ToTStr(pPayChannelElement->GetText());

		if (pPayChannelOrderNumberElement->GetText() != NULL)
			rechargeOrder.m_strPayChannelOrderNumber = CStringUtil::Utf8ToTStr(pPayChannelOrderNumberElement->GetText());

		if (pBankOrderNumberElement->GetText() != NULL)
			rechargeOrder.m_strBankOrderNumber = CStringUtil::Utf8ToTStr(pBankOrderNumberElement->GetText());

		if (pPayerElement->GetText() != NULL)
			rechargeOrder.m_strPayer = CStringUtil::Utf8ToTStr(pPayerElement->GetText());

		if (pDeliveryNoteElement->GetText() != NULL)
			rechargeOrder.m_strDeliveryNote = CStringUtil::Utf8ToTStr(pDeliveryNoteElement->GetText());

		if (pPayNoteElement->GetText() != NULL)
			rechargeOrder.m_strPayNote = CStringUtil::Utf8ToTStr(pPayNoteElement->GetText());

		if (pGifeCodeElement->GetText() != NULL)
			rechargeOrder.m_strGifeCode = CStringUtil::Utf8ToTStr(pGifeCodeElement->GetText());

		if (pRebateRatioElement->GetText() != NULL)
			rechargeOrder.m_fRebateRatio = atof(pRebateRatioElement->GetText());

		TiXmlElement* pSnapshotElement = NULL;
		for (pSnapshotElement = pSnapshotListElement->FirstChildElement("Snapshot"); pSnapshotElement; pSnapshotElement = pSnapshotElement->NextSiblingElement("Snapshot"))
		{
			COrderSnapshot orderSnapshot;
			if (pSnapshotElement->Attribute("type") != NULL)
				orderSnapshot.m_nType = atoi(pSnapshotElement->Attribute("type"));
			if (pSnapshotElement->Attribute("hashFileName") != NULL)
				orderSnapshot.m_strHashFileName = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("hashFileName"));
			if (pSnapshotElement->Attribute("originalFileName"))
				orderSnapshot.m_strOriginalFileName = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("originalFileName"));
			if (pSnapshotElement->Attribute("originalFilePath"))
				orderSnapshot.m_strOriginalFilePath = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("originalFilePath"));

			rechargeOrder.m_vecSnapshot.push_back(orderSnapshot);
		}

		TiXmlElement* pPayOrderElement = NULL;
		for (pPayOrderElement = pPayOrderListElement->FirstChildElement("PayOrder"); pPayOrderElement; pPayOrderElement = pPayOrderElement->NextSiblingElement("PayOrder"))
		{
			CPayOrder payOrder;
			if (pPayOrderElement->Attribute("channelId") != NULL)
				payOrder.m_nChannelId = atoi(pPayOrderElement->Attribute("channelId"));
			if (pPayOrderElement->Attribute("orderId") != NULL)
				payOrder.m_nOrderId = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("orderId"));
			if (pPayOrderElement->Attribute("loginAccount") != NULL)
				payOrder.m_strLoginAccount = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("loginAccount"));
			if (pPayOrderElement->Attribute("userName") != NULL)
				payOrder.m_strUserName = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("userName"));
			if (pPayOrderElement->Attribute("gameName") != NULL)
				payOrder.m_strGameName = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("gameName"));
			if (pPayOrderElement->Attribute("buyAccount") != NULL)
				payOrder.m_strBuyAccount = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("buyAccount"));
			if (pPayOrderElement->Attribute("buyPassword") != NULL)
				payOrder.m_strBuyPassword = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("buyPassword"));
			if (pPayOrderElement->Attribute("productName") != NULL)
				payOrder.m_strProductName = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("productName"));
			if (pPayOrderElement->Attribute("productType") != NULL)
				payOrder.m_strProductType = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("productType"));
			if (pPayOrderElement->Attribute("buyPrice") != NULL)
				payOrder.m_fBuyPrice = atof(pPayOrderElement->Attribute("buyPrice"));
			if (pPayOrderElement->Attribute("buyQuantity") != NULL)
				payOrder.m_nBuyQuantity = atoi(pPayOrderElement->Attribute("buyQuantity"));
			if (pPayOrderElement->Attribute("salePrice") != NULL)
				payOrder.m_nSalePrice = atoi(pPayOrderElement->Attribute("salePrice"));
			if (pPayOrderElement->Attribute("totalAmount") != NULL)
				payOrder.m_fTotalAmount = atof(pPayOrderElement->Attribute("totalAmount"));
			if (pPayOrderElement->Attribute("realAmount") != NULL)
				payOrder.m_fRealPrice = atof(pPayOrderElement->Attribute("realAmount"));
			if (pPayOrderElement->Attribute("profit") != NULL)
				payOrder.m_fProfit = atof(pPayOrderElement->Attribute("profit"));
			if (pPayOrderElement->Attribute("buyTime") != NULL)
				payOrder.m_strBuyTime = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("buyTime"));
			if (pPayOrderElement->Attribute("oprIP") != NULL)
				payOrder.m_strOprIP = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("oprIP"));
			if (pPayOrderElement->Attribute("expirTime") != NULL)
				payOrder.m_strExpirationTime = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("expirTime"));
			if (pPayOrderElement->Attribute("remarks") != NULL)
				payOrder.m_strRemarks = CStringUtil::Utf8ToTStr(pPayOrderElement->Attribute("remarks"));

			vecPayOrder.push_back(payOrder);
		}

		return true;
	}

	static void BuildAddUserLogBs(int nType, const tstring& strTime, const tstring& strMsg, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement typeElement("Type");
		TiXmlElement timeElement("Time");
		TiXmlElement msgElement("Msg");

		InsertTextToEndChild(typeElement, _T("%d"), nType);
		InsertTextToEndChild(timeElement, _T("%s"), strTime.c_str());
		InsertTextToEndChild(msgElement, _T("%s"), strMsg.c_str());
		dataElement.InsertEndChild(typeElement);
		dataElement.InsertEndChild(timeElement);
		dataElement.InsertEndChild(msgElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_UserLog"), dataElement), outByteStream);
	}

	static bool ParseAddUserLogBs(int& nType, tstring& strTime, tstring& strMsg, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_UserLog"))
			return false;

		TiXmlElement* pTypeElement = dataElement.FirstChildElement("Type");
		if (pTypeElement == NULL)
			return false;

		TiXmlElement* pTimeElement = dataElement.FirstChildElement("Time");
		if (pTimeElement == NULL)
			return false;

		TiXmlElement* pMsgElement = dataElement.FirstChildElement("Msg");
		if (pMsgElement == NULL)
			return false;

		if (pTypeElement->GetText() != NULL)
			nType = atoi(pTypeElement->GetText());

		if (pTimeElement->GetText() != NULL)
			strTime = CStringUtil::Utf8ToTStr(pTimeElement->GetText());

		if (pMsgElement->GetText() != NULL)
			strMsg = CStringUtil::Utf8ToTStr(pMsgElement->GetText());

		return true;
	}

	static void BuildGetRechargeOrderBs(const ORDER_QUERY_PARAM& orderQueryParam, int nOffset, int nCount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement orderQueryParamElement("OrderQueryParam");
		TiXmlElement offsetElement("Offset");
		TiXmlElement countElement("Count");

		if (orderQueryParam.spStrNumber != NULL)
		{
			TiXmlElement numberElment("Number");
			InsertTextToEndChild(numberElment, _T("%s"), orderQueryParam.spStrNumber->c_str());
			orderQueryParamElement.InsertEndChild(numberElment);
		}
		if (orderQueryParam.spNCustomerId != NULL)
		{
			TiXmlElement customerIdElement("CustomerId");
			InsertTextToEndChild(customerIdElement, _T("%d"), *orderQueryParam.spNCustomerId);
			orderQueryParamElement.InsertEndChild(customerIdElement);
		}
		if (orderQueryParam.spStrUserName != NULL)
		{
			TiXmlElement userNameElement("UserName");
			InsertTextToEndChild(userNameElement, _T("%s"), orderQueryParam.spStrUserName->c_str());
			orderQueryParamElement.InsertEndChild(userNameElement);
		}
		if (orderQueryParam.spStrState != NULL)
		{
			TiXmlElement stateElement("State");
			InsertTextToEndChild(stateElement, _T("%s"), orderQueryParam.spStrState->c_str());
			orderQueryParamElement.InsertEndChild(stateElement);
		}
		if (orderQueryParam.spStrGameName != NULL)
		{
			TiXmlElement gameNameElement("GameName");
			InsertTextToEndChild(gameNameElement, _T("%s"), orderQueryParam.spStrGameName->c_str());
			orderQueryParamElement.InsertEndChild(gameNameElement);
		}
		if (orderQueryParam.spStrChannel != NULL)
		{
			TiXmlElement channelElement("Channel");
			InsertTextToEndChild(channelElement, _T("%s"), orderQueryParam.spStrChannel->c_str());
			orderQueryParamElement.InsertEndChild(channelElement);
		}
		if (orderQueryParam.spStrDateStart != NULL)
		{
			TiXmlElement dateStartElement("DateStart");
			InsertTextToEndChild(dateStartElement, _T("%s"), orderQueryParam.spStrDateStart->c_str());
			orderQueryParamElement.InsertEndChild(dateStartElement);
		}
		if (orderQueryParam.spStrDateEnd != NULL)
		{
			TiXmlElement dateEndElement("DateEnd");
			InsertTextToEndChild(dateEndElement, _T("%s"), orderQueryParam.spStrDateEnd->c_str());
			orderQueryParamElement.InsertEndChild(dateEndElement);
		}

		InsertTextToEndChild(offsetElement, _T("%d"), nOffset);
		InsertTextToEndChild(countElement, _T("%d"), nCount);
		dataElement.InsertEndChild(orderQueryParamElement);
		dataElement.InsertEndChild(offsetElement);
		dataElement.InsertEndChild(countElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_RechargeOrder"), dataElement), outByteStream);
	}

	static bool ParseGetRechargeOrderBs(ORDER_QUERY_PARAM& orderQueryParam, int& nOffset, int& nCount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_RechargeOrder"))
			return false;

		TiXmlElement* pOrderQueryParamElement = dataElement.FirstChildElement("OrderQueryParam");
		if (pOrderQueryParamElement == NULL)
			return false;

		TiXmlElement* pOffsetElement = dataElement.FirstChildElement("Offset");
		if (pOffsetElement == NULL)
			return false;

		TiXmlElement* pCountElement = dataElement.FirstChildElement("Count");
		if (pCountElement == NULL)
			return false;

		if (pOffsetElement->GetText() != NULL)
			nOffset = atoi(pOffsetElement->GetText());

		if (pCountElement->GetText() != NULL)
			nCount = atoi(pCountElement->GetText());

		if (pOrderQueryParamElement != NULL)
		{
			TiXmlElement* pNumberElement = pOrderQueryParamElement->FirstChildElement("Number");
			if (pNumberElement != NULL && pNumberElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrNumber = spStr;
				*orderQueryParam.spStrNumber = CStringUtil::Utf8ToTStr(pNumberElement->GetText());
			}

			TiXmlElement* pCustomerIdElement = pOrderQueryParamElement->FirstChildElement("CustomerId");
			if (pCustomerIdElement != NULL && pCustomerIdElement->GetText() != NULL)
			{
				std::shared_ptr<int> spInt(new int());
				orderQueryParam.spNCustomerId = spInt;
				*orderQueryParam.spNCustomerId = atoi(pCustomerIdElement->GetText());
			}

			TiXmlElement* pUserNameElement = pOrderQueryParamElement->FirstChildElement("UserName");
			if (pUserNameElement != NULL && pUserNameElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrUserName = spStr;
				*orderQueryParam.spStrUserName = CStringUtil::Utf8ToTStr(pUserNameElement->GetText());
			}

			TiXmlElement* pStateElement = pOrderQueryParamElement->FirstChildElement("State");
			if (pStateElement != NULL && pStateElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrState = spStr;
				*orderQueryParam.spStrState = CStringUtil::Utf8ToTStr(pStateElement->GetText());
			}

			TiXmlElement* pGameNameElement = pOrderQueryParamElement->FirstChildElement("GameName");
			if (pGameNameElement != NULL && pGameNameElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrGameName = spStr;
				*orderQueryParam.spStrGameName = CStringUtil::Utf8ToTStr(pGameNameElement->GetText());
			}

			TiXmlElement* pChannelElement = pOrderQueryParamElement->FirstChildElement("Channel");
			if (pChannelElement != NULL && pChannelElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrChannel = spStr;
				*orderQueryParam.spStrChannel = CStringUtil::Utf8ToTStr(pChannelElement->GetText());
			}

			TiXmlElement* pDateStartElement = pOrderQueryParamElement->FirstChildElement("DateStart");
			if (pDateStartElement != NULL && pDateStartElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrDateStart = spStr;
				*orderQueryParam.spStrDateStart = CStringUtil::Utf8ToTStr(pDateStartElement->GetText());
			}

			TiXmlElement* pDateEndElement = pOrderQueryParamElement->FirstChildElement("DateEnd");
			if (pDateEndElement != NULL && pDateEndElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrDateEnd = spStr;
				*orderQueryParam.spStrDateEnd = CStringUtil::Utf8ToTStr(pDateEndElement->GetText());
			}
		}

		return true;
	}

	static void BuildUpdateRechargeOrderBs(int nOrderId, double fRebateRatio, const std::vector<tstring>& vecPlatformOrderNumber, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement ordreIdElement("OrderId");
		TiXmlElement rebateRatioElement("RebateRatio");

		TiXmlElement platformOrderNumberListElement("PlatformOrderNumberList");
		for (int i = 0; i < vecPlatformOrderNumber.size(); i++)
		{
			TiXmlElement platformOrderNumberElement("PlatformOrderNumber");
			InsertTextToEndChild(platformOrderNumberElement, _T("%s"), vecPlatformOrderNumber[i].c_str());

			platformOrderNumberListElement.InsertEndChild(platformOrderNumberElement);
		}

		InsertTextToEndChild(ordreIdElement, _T("%d"), nOrderId);
		InsertTextToEndChild(rebateRatioElement, _T("%f"), fRebateRatio);

		dataElement.InsertEndChild(ordreIdElement);
		dataElement.InsertEndChild(rebateRatioElement);
		dataElement.InsertEndChild(platformOrderNumberListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_RechargeOrder"), dataElement), outByteStream);
	}

	static bool ParseUpdateRechargeOrderBs(int& nOrderId, double& fRebateRatio, std::vector<tstring>& vecPlatformOrderNumber, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_RechargeOrder"))
			return false;

		TiXmlElement* pIdElement = dataElement.FirstChildElement("OrderId");
		if (pIdElement == NULL)
			return false;

		TiXmlElement* pRebateRatioElement = dataElement.FirstChildElement("RebateRatio");
		if (pRebateRatioElement == NULL)
			return false;

		if (pIdElement->GetText() != NULL)
			nOrderId = atoi(pIdElement->GetText());

		if (pRebateRatioElement->GetText() != NULL)
			fRebateRatio = atof(pRebateRatioElement->GetText());

		TiXmlElement* pPlatformOrderNumberListElement = dataElement.FirstChildElement("PlatformOrderNumberList");
		if (pPlatformOrderNumberListElement != NULL)
		{
			TiXmlElement* pPlatformOrderNumberElement = NULL;
			for (pPlatformOrderNumberElement = pPlatformOrderNumberListElement->FirstChildElement("PlatformOrderNumber"); pPlatformOrderNumberElement; pPlatformOrderNumberElement = pPlatformOrderNumberElement->NextSiblingElement("PlatformOrderNumber"))
			{
				if (pPlatformOrderNumberElement->GetText() != NULL)
					vecPlatformOrderNumber.push_back(CStringUtil::Utf8ToTStr(pPlatformOrderNumberElement->GetText()));
			}
		}

		return true;
	}

	static void BuildGetRechargeOrderExportBs(const ORDER_QUERY_PARAM& orderQueryParam, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement orderQueryParamElement("OrderQueryParam");

		if (orderQueryParam.spStrNumber != NULL)
		{
			TiXmlElement numberElment("Number");
			InsertTextToEndChild(numberElment, _T("%s"), orderQueryParam.spStrNumber->c_str());
			orderQueryParamElement.InsertEndChild(numberElment);
		}
		if (orderQueryParam.spNCustomerId != NULL)
		{
			TiXmlElement customerIdElement("CustomerId");
			InsertTextToEndChild(customerIdElement, _T("%d"), *orderQueryParam.spNCustomerId);
			orderQueryParamElement.InsertEndChild(customerIdElement);
		}
		if (orderQueryParam.spStrUserName != NULL)
		{
			TiXmlElement userNameElement("UserName");
			InsertTextToEndChild(userNameElement, _T("%s"), orderQueryParam.spStrUserName->c_str());
			orderQueryParamElement.InsertEndChild(userNameElement);
		}
		if (orderQueryParam.spStrState != NULL)
		{
			TiXmlElement stateElement("State");
			InsertTextToEndChild(stateElement, _T("%s"), orderQueryParam.spStrState->c_str());
			orderQueryParamElement.InsertEndChild(stateElement);
		}
		if (orderQueryParam.spStrGameName != NULL)
		{
			TiXmlElement gameNameElement("GameName");
			InsertTextToEndChild(gameNameElement, _T("%s"), orderQueryParam.spStrGameName->c_str());
			orderQueryParamElement.InsertEndChild(gameNameElement);
		}
		if (orderQueryParam.spStrChannel != NULL)
		{
			TiXmlElement channelElement("Channel");
			InsertTextToEndChild(channelElement, _T("%s"), orderQueryParam.spStrChannel->c_str());
			orderQueryParamElement.InsertEndChild(channelElement);
		}
		if (orderQueryParam.spStrDateStart != NULL)
		{
			TiXmlElement dateStartElement("DateStart");
			InsertTextToEndChild(dateStartElement, _T("%s"), orderQueryParam.spStrDateStart->c_str());
			orderQueryParamElement.InsertEndChild(dateStartElement);
		}
		if (orderQueryParam.spStrDateEnd != NULL)
		{
			TiXmlElement dateEndElement("DateEnd");
			InsertTextToEndChild(dateEndElement, _T("%s"), orderQueryParam.spStrDateEnd->c_str());
			orderQueryParamElement.InsertEndChild(dateEndElement);
		}

		dataElement.InsertEndChild(orderQueryParamElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_RechargeOrder_Export"), dataElement), outByteStream);
	}

	static bool ParseGetRechargeOrderExportBs(ORDER_QUERY_PARAM& orderQueryParam, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_RechargeOrder_Export"))
			return false;

		TiXmlElement* pOrderQueryParamElement = dataElement.FirstChildElement("OrderQueryParam");
		if (pOrderQueryParamElement == NULL)
			return false;

		if (pOrderQueryParamElement != NULL)
		{
			TiXmlElement* pNumberElement = pOrderQueryParamElement->FirstChildElement("Number");
			if (pNumberElement != NULL && pNumberElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrNumber = spStr;
				*orderQueryParam.spStrNumber = CStringUtil::Utf8ToTStr(pNumberElement->GetText());
			}

			TiXmlElement* pCustomerIdElement = pOrderQueryParamElement->FirstChildElement("CustomerId");
			if (pCustomerIdElement != NULL && pCustomerIdElement->GetText() != NULL)
			{
				std::shared_ptr<int> spInt(new int());
				orderQueryParam.spNCustomerId = spInt;
				*orderQueryParam.spNCustomerId = atoi(pCustomerIdElement->GetText());
			}

			TiXmlElement* pUserNameElement = pOrderQueryParamElement->FirstChildElement("UserName");
			if (pUserNameElement != NULL && pUserNameElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrUserName = spStr;
				*orderQueryParam.spStrUserName = CStringUtil::Utf8ToTStr(pUserNameElement->GetText());
			}

			TiXmlElement* pStateElement = pOrderQueryParamElement->FirstChildElement("State");
			if (pStateElement != NULL && pStateElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrState = spStr;
				*orderQueryParam.spStrState = CStringUtil::Utf8ToTStr(pStateElement->GetText());
			}

			TiXmlElement* pGameNameElement = pOrderQueryParamElement->FirstChildElement("GameName");
			if (pGameNameElement != NULL && pGameNameElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrGameName = spStr;
				*orderQueryParam.spStrGameName = CStringUtil::Utf8ToTStr(pGameNameElement->GetText());
			}

			TiXmlElement* pChannelElement = pOrderQueryParamElement->FirstChildElement("Channel");
			if (pChannelElement != NULL && pChannelElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrChannel = spStr;
				*orderQueryParam.spStrChannel = CStringUtil::Utf8ToTStr(pChannelElement->GetText());
			}

			TiXmlElement* pDateStartElement = pOrderQueryParamElement->FirstChildElement("DateStart");
			if (pDateStartElement != NULL && pDateStartElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrDateStart = spStr;
				*orderQueryParam.spStrDateStart = CStringUtil::Utf8ToTStr(pDateStartElement->GetText());
			}

			TiXmlElement* pDateEndElement = pOrderQueryParamElement->FirstChildElement("DateEnd");
			if (pDateEndElement != NULL && pDateEndElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				orderQueryParam.spStrDateEnd = spStr;
				*orderQueryParam.spStrDateEnd = CStringUtil::Utf8ToTStr(pDateEndElement->GetText());
			}
		}

		return true;
	}

	static void BuildGameAccountImportBs(const std::vector<CGameAccount>& vecGameAccount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		TiXmlElement gameAccountListElement("GameAccountList");
		for (int i = 0; i < vecGameAccount.size(); i++)
		{
			TiXmlElement gameAccountElement("GameAccount");
			gameAccountElement.SetAttribute("channelId", vecGameAccount[i].m_nId);
			gameAccountElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecGameAccount[i].m_strUserName).c_str());
			gameAccountElement.SetAttribute("password", CStringUtil::TStrToUtf8(vecGameAccount[i].m_strPassword).c_str());
			gameAccountElement.SetAttribute("gameName", CStringUtil::TStrToUtf8(vecGameAccount[i].m_strGameName).c_str());

			gameAccountListElement.InsertEndChild(gameAccountElement);
		}

		dataElement.InsertEndChild(gameAccountListElement);

		BuildXmlBs(BuildProtoXmlString(_T("GameAccount_Import"), dataElement), outByteStream);
	}

	static bool ParseGameAccountImportBs(std::vector<CGameAccount>& vecGameAccout, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("GameAccount_Import"))
			return false;

		TiXmlElement* pGameAccountListElement = dataElement.FirstChildElement("GameAccountList");
		if (pGameAccountListElement != NULL)
		{
			TiXmlElement* pGameAccountElement = NULL;
			for (pGameAccountElement = pGameAccountListElement->FirstChildElement("GameAccount"); pGameAccountElement; pGameAccountElement = pGameAccountElement->NextSiblingElement("GameAccount"))
			{
				CGameAccount gameAccount;
				if (pGameAccountElement->Attribute("userName") != NULL)
					gameAccount.m_strUserName = CStringUtil::Utf8ToTStr(pGameAccountElement->Attribute("userName"));
				if (pGameAccountElement->Attribute("password") != NULL)
					gameAccount.m_strPassword = CStringUtil::Utf8ToTStr(pGameAccountElement->Attribute("password"));
				if (pGameAccountElement->Attribute("gameName") != NULL)
					gameAccount.m_strGameName = CStringUtil::Utf8ToTStr(pGameAccountElement->Attribute("gameName"));
				if (pGameAccountElement->Attribute("channelId") != NULL)
					gameAccount.m_nId = atoi(pGameAccountElement->Attribute("channelId"));

					vecGameAccout.push_back(gameAccount);
			}
		}

		return true;
	}

	static void BuildGetUserLogBs(const LOG_QUERY_PARAM& logQueryParam, int nOffset, int nCount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement logQueryParamElement("LogQueryParam");
		TiXmlElement offsetElement("Offset");
		TiXmlElement countElement("Count");

		if (logQueryParam.spNUserId != NULL)
		{
			TiXmlElement userIdElment("UserId");
			InsertTextToEndChild(userIdElment, _T("%d"), *logQueryParam.spNUserId);
			logQueryParamElement.InsertEndChild(userIdElment);
		}
		if (logQueryParam.spStrDuration != NULL)
		{
			TiXmlElement durationElement("Duration");
			InsertTextToEndChild(durationElement, _T("%s"), logQueryParam.spStrDuration->c_str());
			logQueryParamElement.InsertEndChild(durationElement);
		}
		if (logQueryParam.spStrMsg != NULL)
		{
			TiXmlElement msgElement("Msg");
			InsertTextToEndChild(msgElement, _T("%s"), logQueryParam.spStrMsg->c_str());
			logQueryParamElement.InsertEndChild(msgElement);
		}

		InsertTextToEndChild(offsetElement, _T("%d"), nOffset);
		InsertTextToEndChild(countElement, _T("%d"), nCount);
		dataElement.InsertEndChild(logQueryParamElement);
		dataElement.InsertEndChild(offsetElement);
		dataElement.InsertEndChild(countElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_UserLog"), dataElement), outByteStream);
	}

	static bool ParseGetUserLogBs(LOG_QUERY_PARAM& logQueryParam, int& nOffset, int& nCount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_UserLog"))
			return false;

		TiXmlElement* pLogQueryParamElement = dataElement.FirstChildElement("LogQueryParam");
		if (pLogQueryParamElement == NULL)
			return false;

		TiXmlElement* pOffsetElement = dataElement.FirstChildElement("Offset");
		if (pOffsetElement == NULL)
			return false;

		TiXmlElement* pCountElement = dataElement.FirstChildElement("Count");
		if (pCountElement == NULL)
			return false;

		if (pOffsetElement->GetText() != NULL)
			nOffset = atoi(pOffsetElement->GetText());

		if (pCountElement->GetText() != NULL)
			nCount = atoi(pCountElement->GetText());

		if (pLogQueryParamElement != NULL)
		{
			TiXmlElement* pUserIdElement = pLogQueryParamElement->FirstChildElement("UserId");
			if (pUserIdElement != NULL && pUserIdElement->GetText() != NULL)
			{
				std::shared_ptr<int> spInt(new int());
				logQueryParam.spNUserId = spInt;
				*logQueryParam.spNUserId = atoi(pUserIdElement->GetText());
			}

			TiXmlElement* pDurationElement = pLogQueryParamElement->FirstChildElement("Duration");
			if (pDurationElement != NULL && pDurationElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				logQueryParam.spStrDuration = spStr;
				*logQueryParam.spStrDuration = CStringUtil::Utf8ToTStr(pDurationElement->GetText());
			}

			TiXmlElement* pMsgElement = pLogQueryParamElement->FirstChildElement("Msg");
			if (pMsgElement != NULL && pMsgElement->GetText() != NULL)
			{
				std::shared_ptr<tstring> spStr(new tstring());
				logQueryParam.spStrMsg = spStr;
				*logQueryParam.spStrMsg = CStringUtil::Utf8ToTStr(pMsgElement->GetText());
			}
		}

		return true;
	}

	static void BuildGetUserBs(CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		BuildXmlBs(BuildProtoXmlString(_T("Get_User"), dataElement), outByteStream);
	}

	static void BuildGetUserLogLocation(int nUserLogId, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement gameAccountIdElement("UserLogId");

		InsertTextToEndChild(gameAccountIdElement, _T("%d"), nUserLogId);
		dataElement.InsertEndChild(gameAccountIdElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_UserLog_Location"), dataElement), outByteStream);
	}

	static bool ParseGetUserLogLocationBs(int& nUserLogId, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_UserLog_Location"))
			return false;

		TiXmlElement* pUserLogIdElement = dataElement.FirstChildElement("UserLogId");
		if (pUserLogIdElement == NULL)
			return false;

		if (pUserLogIdElement->GetText() != NULL)
			nUserLogId = atoi(pUserLogIdElement->GetText());

		return true;
	}

	static void BuildAddUserBs(const CUser& user, const tstring& strPassword, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement userNameElement("UserName");
		TiXmlElement roleNameElement("RoleName");
		TiXmlElement emailElement("Email");
		TiXmlElement mobileElement("Mobile");
		TiXmlElement passwordElement("Password");

		InsertTextToEndChild(userNameElement, _T("%s"), user.m_strUserName.c_str());
		InsertTextToEndChild(roleNameElement, _T("%s"), user.m_strRoleName.c_str());
		InsertTextToEndChild(emailElement, _T("%s"), user.m_strEmail.c_str());
		InsertTextToEndChild(mobileElement, _T("%s"), user.m_strMobile.c_str());
		InsertTextToEndChild(passwordElement, _T("%s"), strPassword.c_str());
		dataElement.InsertEndChild(userNameElement);
		dataElement.InsertEndChild(roleNameElement);
		dataElement.InsertEndChild(emailElement);
		dataElement.InsertEndChild(mobileElement);
		dataElement.InsertEndChild(passwordElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_User"), dataElement), outByteStream);
	}

	static bool ParseAddUserBs(CUser& user, tstring& strPassword, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_User"))
			return false;

		TiXmlElement* pUserNameElement = dataElement.FirstChildElement("UserName");
		if (pUserNameElement == NULL)
			return false;

		TiXmlElement* pRoleNameElement = dataElement.FirstChildElement("RoleName");
		if (pRoleNameElement == NULL)
			return false;

		TiXmlElement* pEmailElement = dataElement.FirstChildElement("Email");
		if (pEmailElement == NULL)
			return false;

		TiXmlElement* pMobileElement = dataElement.FirstChildElement("Mobile");
		if (pMobileElement == NULL)
			return false;

		TiXmlElement* pPasswordElement = dataElement.FirstChildElement("Password");
		if (pPasswordElement == NULL)
			return false;

		if (pUserNameElement->GetText() != NULL)
			user.m_strUserName = CStringUtil::Utf8ToTStr(pUserNameElement->GetText());

		if (pRoleNameElement->GetText() != NULL)
			user.m_strRoleName = CStringUtil::Utf8ToTStr(pRoleNameElement->GetText());

		if (pEmailElement->GetText() != NULL)
			user.m_strEmail = CStringUtil::Utf8ToTStr(pEmailElement->GetText());

		if (pMobileElement->GetText() != NULL)
			user.m_strMobile = CStringUtil::Utf8ToTStr(pMobileElement->GetText());

		if (pPasswordElement->GetText() != NULL)
			strPassword = CStringUtil::Utf8ToTStr(pPasswordElement->GetText());

		return true;
	}

	static void BuildInitTianHongGameListBs(CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		BuildXmlBs(BuildProtoXmlString(_T("Init_TianHong_GameList"), dataElement), outByteStream);
	}

	static bool ParseInitTianHongGameListBs(CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Init_TianHong_GameList"))
			return false;

		return true;
	}

	static void BuildAddTianHongGameInfoBs(const tstring& strGameName, int nType, int nBill, const std::map<int, int> mapItem, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement gameDataElement("GameData");

		gameDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strGameName).c_str());
		gameDataElement.SetAttribute("bill", nBill);
		gameDataElement.SetAttribute("type", nType);

		for (std::map<int, int>::const_iterator mapIter = mapItem.cbegin(); mapIter != mapItem.cend(); ++mapIter)
		{
			TiXmlElement sepElement("GameSepData");
			sepElement.SetAttribute("first", mapIter->first);
			sepElement.SetAttribute("second", mapIter->second);
			gameDataElement.InsertEndChild(sepElement);
		}

		dataElement.InsertEndChild(gameDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_TianHong_GameInfo"), dataElement), outByteStream);
	}

	static bool ParseAddTianHongGameInfoBs(tstring& strGameName, int& nType, int& nBill, std::map<int, int>& mapItem, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_TianHong_GameInfo"))
			return false;

		TiXmlElement* pGameDataElement = dataElement.FirstChildElement("GameData");
		if (pGameDataElement == NULL)
			return false;

		if (pGameDataElement->Attribute("name") != NULL)
			strGameName = CStringUtil::Utf8ToTStr(pGameDataElement->Attribute("name"));

		if (pGameDataElement->Attribute("type") != NULL)
			nType = atoi(pGameDataElement->Attribute("type"));

		if (pGameDataElement->Attribute("bill") != NULL)
			nBill = atoi(pGameDataElement->Attribute("bill"));

		TiXmlElement* pSepElement = NULL;
		for (pSepElement = pGameDataElement->FirstChildElement("GameSepData"); pSepElement; pSepElement = pSepElement->NextSiblingElement("GameSepData"))
		{
			int nFirst, nSecond;
			if (pSepElement->Attribute("first") != NULL)
			{
				nFirst = atoi(pSepElement->Attribute("first"));
			}
			if (pSepElement->Attribute("second") != NULL)
			{
				nSecond = atoi(pSepElement->Attribute("second"));
			}
			mapItem.insert(std::pair<int, int>(nFirst, nSecond));
		}

		return true;
	}

	static void BuildUpdateTianHongGameInfoBs(const tstring& strGameName, int nType, int nBill, const std::map<int, int> mapItem, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement gameDataElement("GameData");

		gameDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strGameName).c_str());
		gameDataElement.SetAttribute("bill", nBill);
		gameDataElement.SetAttribute("type", nType);

		for (std::map<int, int>::const_iterator mapIter = mapItem.cbegin(); mapIter != mapItem.cend(); ++mapIter)
		{
			TiXmlElement sepElement("GameSepData");
			sepElement.SetAttribute("first", mapIter->first);
			sepElement.SetAttribute("second", mapIter->second);
			gameDataElement.InsertEndChild(sepElement);
		}

		dataElement.InsertEndChild(gameDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_TianHong_GameInfo"), dataElement), outByteStream);
	}

	static bool ParseUpdateTianHongGameInfoBs(tstring& strGameName, int& nType, int& nBill, std::map<int, int>& mapItem, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_TianHong_GameInfo"))
			return false;

		TiXmlElement* pGameDataElement = dataElement.FirstChildElement("GameData");
		if (pGameDataElement == NULL)
			return false;

		if (pGameDataElement->Attribute("name") != NULL)
			strGameName = CStringUtil::Utf8ToTStr(pGameDataElement->Attribute("name"));

		if (pGameDataElement->Attribute("type") != NULL)
			nType = atoi(pGameDataElement->Attribute("type"));

		if (pGameDataElement->Attribute("bill") != NULL)
			nBill = atoi(pGameDataElement->Attribute("bill"));

		TiXmlElement* pSepElement = NULL;
		for (pSepElement = pGameDataElement->FirstChildElement("GameSepData"); pSepElement; pSepElement = pSepElement->NextSiblingElement("GameSepData"))
		{
			int nFirst, nSecond;
			if (pSepElement->Attribute("first") != NULL)
			{
				nFirst = atoi(pSepElement->Attribute("first"));
			}
			if (pSepElement->Attribute("second") != NULL)
			{
				nSecond = atoi(pSepElement->Attribute("second"));
			}
			mapItem.insert(std::pair<int, int>(nFirst, nSecond));
		}

		return true;
	}

	static void BuildDeleteTianHongGameInfoBs(const tstring& strGameName, int nType, int nBill, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement gameDataElement("GameData");

		gameDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strGameName).c_str());
		gameDataElement.SetAttribute("bill", nBill);
		gameDataElement.SetAttribute("type", nType);

		dataElement.InsertEndChild(gameDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_TianHong_GameInfo"), dataElement), outByteStream);
	}

	static bool ParseDeleteTianHongGameInfoBs(tstring& strGameName, int& nType, int& nBill, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_TianHong_GameInfo"))
			return false;

		TiXmlElement* pGameDataElement = dataElement.FirstChildElement("GameData");
		if (pGameDataElement == NULL)
			return false;

		if (pGameDataElement->Attribute("name") != NULL)
			strGameName = CStringUtil::Utf8ToTStr(pGameDataElement->Attribute("name"));

		if (pGameDataElement->Attribute("type") != NULL)
			nType = atoi(pGameDataElement->Attribute("type"));

		if (pGameDataElement->Attribute("bill") != NULL)
			nBill = atoi(pGameDataElement->Attribute("bill"));

		return true;
	}

	static void BuildGetSmartOrderResultBs(const tstring& strGameName, int nType, int nBill, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement gameDataElement("GameData");

		gameDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strGameName).c_str());
		gameDataElement.SetAttribute("bill", nBill);
		gameDataElement.SetAttribute("type", nType);

		dataElement.InsertEndChild(gameDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_SmartOrder_Result"), dataElement), outByteStream);
	}

	static bool ParseGetSmartOrderResultBs(tstring& strGameName, int& nType, int& nBill, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_SmartOrder_Result"))
			return false;

		TiXmlElement* pGameDataElement = dataElement.FirstChildElement("GameData");
		if (pGameDataElement == NULL)
			return false;

		if (pGameDataElement->Attribute("name") != NULL)
			strGameName = CStringUtil::Utf8ToTStr(pGameDataElement->Attribute("name"));

		if (pGameDataElement->Attribute("type") != NULL)
			nType = atoi(pGameDataElement->Attribute("type"));

		if (pGameDataElement->Attribute("bill") != NULL)
			nBill = atoi(pGameDataElement->Attribute("bill"));

		return true;
	}

	static void BuildGetReportDataBs(const tstring& strBeginTime, const tstring& strEndTime, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement beginTimeElement("BeginTime");
		TiXmlElement endTimeElement("EndTime");

		InsertTextToEndChild(beginTimeElement, _T("%s"), strBeginTime.c_str());
		InsertTextToEndChild(endTimeElement, _T("%s"), strEndTime.c_str());

		dataElement.InsertEndChild(beginTimeElement);
		dataElement.InsertEndChild(endTimeElement);		

		BuildXmlBs(BuildProtoXmlString(_T("Get_Report_Data"), dataElement), outByteStream);
	}

	static bool ParseGetReportDataBs(tstring& strBeginTime, tstring& strEndTime, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Report_Data"))
			return false;

		TiXmlElement* pBeginTimeElement = dataElement.FirstChildElement("BeginTime");
		if (pBeginTimeElement == NULL)
			return false;

		TiXmlElement* pEndTimeElement = dataElement.FirstChildElement("EndTime");
		if (pEndTimeElement == NULL)
			return false;

		if (pBeginTimeElement->GetText() != NULL)
			strBeginTime = CStringUtil::Utf8ToTStr(pBeginTimeElement->GetText());

		if (pEndTimeElement->GetText() != NULL)
			strEndTime = CStringUtil::Utf8ToTStr(pEndTimeElement->GetText());

		return true;
	}

	static void BuildGetReportCardListBs(CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		BuildXmlBs(BuildProtoXmlString(_T("Get_BusinessCard_List"), dataElement), outByteStream);
	}

	static bool ParseGetReportCardListBs(CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_BusinessCard_List"))
			return false;

		return true;
	}

	static void BuildAddReportCardInfoBs(const tstring& strCardName, int nValue, double fRatio, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement cardDataElement("CardData");

		cardDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strCardName).c_str());
		cardDataElement.SetAttribute("value", nValue);
		cardDataElement.SetDoubleAttribute("ratio", fRatio);

		dataElement.InsertEndChild(cardDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Report_CardInfo"), dataElement), outByteStream);
	}

	static bool ParseAddReportCardInfoBs(tstring& strCardName, int& nValue, double& fRatio, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Report_CardInfo"))
			return false;

		TiXmlElement* pCardDataElement = dataElement.FirstChildElement("CardData");
		if (pCardDataElement == NULL)
			return false;

		if (pCardDataElement->Attribute("name") != NULL)
			strCardName = CStringUtil::Utf8ToTStr(pCardDataElement->Attribute("name"));

		if (pCardDataElement->Attribute("value") != NULL)
			nValue = atoi(pCardDataElement->Attribute("value"));

		if (pCardDataElement->Attribute("ratio") != NULL)
			fRatio = atof(pCardDataElement->Attribute("ratio"));

		return true;
	}

	static void BuildUpdateReportCardNameBs(const tstring& strOldName, const tstring& strNewName, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement cardDataElement("CardData");

		cardDataElement.SetAttribute("oldName", CStringUtil::TStrToUtf8(strOldName).c_str());
		cardDataElement.SetAttribute("newName", CStringUtil::TStrToUtf8(strNewName).c_str());

		dataElement.InsertEndChild(cardDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Report_CardName"), dataElement), outByteStream);
	}

	static bool ParseUpdateReportCardNameBs(tstring& strOldName, tstring& strNewName, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Report_CardName"))
			return false;

		TiXmlElement* pCardDataElement = dataElement.FirstChildElement("CardData");
		if (pCardDataElement == NULL)
			return false;

		if (pCardDataElement->Attribute("oldName") != NULL)
			strOldName = CStringUtil::Utf8ToTStr(pCardDataElement->Attribute("oldName"));

		if (pCardDataElement->Attribute("newName") != NULL)
			strNewName = CStringUtil::Utf8ToTStr(pCardDataElement->Attribute("newName"));

		return true;
	}

	static void BuildUpdateReportCardInfoBs(const tstring& strCardName, int nValue, double fRatio, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement cardDataElement("CardData");

		cardDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strCardName).c_str());
		cardDataElement.SetAttribute("value", nValue);
		cardDataElement.SetDoubleAttribute("ratio", fRatio);

		dataElement.InsertEndChild(cardDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Report_CardInfo"), dataElement), outByteStream);
	}

	static bool ParseUpdateReportCardInfoBs(tstring& strCardName, int& nValue, double& fRatio, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Report_CardInfo"))
			return false;

		TiXmlElement* pCardDataElement = dataElement.FirstChildElement("CardData");
		if (pCardDataElement == NULL)
			return false;

		if (pCardDataElement->Attribute("name") != NULL)
			strCardName = CStringUtil::Utf8ToTStr(pCardDataElement->Attribute("name"));

		if (pCardDataElement->Attribute("value") != NULL)
			nValue = atoi(pCardDataElement->Attribute("value"));

		if (pCardDataElement->Attribute("ratio") != NULL)
			fRatio = atof(pCardDataElement->Attribute("ratio"));

		return true;
	}

	static void BuildDeleteReportCardInfoBs(const tstring& strCardName, int nValue, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement cardDataElement("CardData");

		cardDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strCardName).c_str());
		cardDataElement.SetAttribute("value", nValue);

		dataElement.InsertEndChild(cardDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Report_CardInfo"), dataElement), outByteStream);
	}

	static bool ParseDeleteReportCardInfoBs(tstring& strCardName, int& nValue, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Report_CardInfo"))
			return false;

		TiXmlElement* pCardDataElement = dataElement.FirstChildElement("CardData");
		if (pCardDataElement == NULL)
			return false;

		if (pCardDataElement->Attribute("name") != NULL)
			strCardName = CStringUtil::Utf8ToTStr(pCardDataElement->Attribute("name"));

		if (pCardDataElement->Attribute("value") != NULL)
			nValue = atoi(pCardDataElement->Attribute("value"));

		return true;
	}

	static void BuildGetReportChannelListBs(CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		BuildXmlBs(BuildProtoXmlString(_T("Get_Channel_List"), dataElement), outByteStream);
	}

	static bool ParseGetReportChannelListBs(CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Channel_List"))
			return false;

		return true;
	}

	static void BuildAddReportChannelInfoBs(const tstring& strChannelName, double fRatio, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement channelDataElement("ChannelData");

		channelDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strChannelName).c_str());
		channelDataElement.SetDoubleAttribute("ratio", fRatio);

		dataElement.InsertEndChild(channelDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Report_ChannelInfo"), dataElement), outByteStream);
	}

	static bool ParseAddReportChannelInfoBs(tstring& strChannelName, double& fRatio, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Report_ChannelInfo"))
			return false;

		TiXmlElement* pChannelDataElement = dataElement.FirstChildElement("ChannelData");
		if (pChannelDataElement == NULL)
			return false;

		if (pChannelDataElement->Attribute("name") != NULL)
			strChannelName = CStringUtil::Utf8ToTStr(pChannelDataElement->Attribute("name"));

		if (pChannelDataElement->Attribute("ratio") != NULL)
			fRatio = atof(pChannelDataElement->Attribute("ratio"));

		return true;
	}

	static void BuildUpdateReportChannelInfoBs(const tstring& strChannelName, double fRatio, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement channelDataElement("ChannelData");

		channelDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strChannelName).c_str());
		channelDataElement.SetDoubleAttribute("ratio", fRatio);

		dataElement.InsertEndChild(channelDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Report_ChannelInfo"), dataElement), outByteStream);
	}

	static bool ParseUpdateReportChannelInfoBs(tstring& strPlatformName, double& fRatio, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Report_ChannelInfo"))
			return false;

		TiXmlElement* pChannelDataElement = dataElement.FirstChildElement("ChannelData");
		if (pChannelDataElement == NULL)
			return false;

		if (pChannelDataElement->Attribute("name") != NULL)
			strPlatformName = CStringUtil::Utf8ToTStr(pChannelDataElement->Attribute("name"));

		if (pChannelDataElement->Attribute("ratio") != NULL)
			fRatio = atof(pChannelDataElement->Attribute("ratio"));

		return true;
	}

	static void BuildDeleteReportChannelInfoBs(const tstring& strChannelName, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement channelDataElement("ChannelData");

		channelDataElement.SetAttribute("name", CStringUtil::TStrToUtf8(strChannelName).c_str());

		dataElement.InsertEndChild(channelDataElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Report_ChannelInfo"), dataElement), outByteStream);
	}

	static bool ParseDeleteReportChannelInfoBs(tstring& strChannelName, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Report_ChannelInfo"))
			return false;

		TiXmlElement* pChannelDataElement = dataElement.FirstChildElement("ChannelData");
		if (pChannelDataElement == NULL)
			return false;

		if (pChannelDataElement->Attribute("name") != NULL)
			strChannelName = CStringUtil::Utf8ToTStr(pChannelDataElement->Attribute("name"));

		return true;
	}

	static void BuildGetBackgroundAccountBs(CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		BuildXmlBs(BuildProtoXmlString(_T("Get_Background_Account"), dataElement), outByteStream);
	}

	static bool ParseGetBackgroundAccountBs(CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Background_Account"))
			return false;

		return true;
	}

	static void BuildGetBackgroundAccountInfoBs(CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		BuildXmlBs(BuildProtoXmlString(_T("Get_Background_Account_Info"), dataElement), outByteStream);
	}

	static bool ParseGetBackgroundAccountInfoBs(CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Background_Account_Info"))
			return false;

		return true;
	}

	static void BuildAddBackgroundAccountInfoBs(const BACKSTAGE_ACCOUNT_ATTRIBUTE& accountInfo, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement accountInfoElement("AccountInfo");

		accountInfoElement.SetAttribute("emType", accountInfo.emType);
		accountInfoElement.SetAttribute("type", CStringUtil::TStrToUtf8(accountInfo.strType).c_str());
		accountInfoElement.SetAttribute("subType", CStringUtil::TStrToUtf8(accountInfo.strSubType).c_str());
		accountInfoElement.SetAttribute("userName", CStringUtil::TStrToUtf8(accountInfo.strUserName).c_str());
		accountInfoElement.SetAttribute("password", CStringUtil::TStrToUtf8(accountInfo.strPassword).c_str());

		dataElement.InsertEndChild(accountInfoElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Background_Account_Info"), dataElement), outByteStream);
	}

	static bool ParseAddBackgroundAccountInfoBs(BACKSTAGE_ACCOUNT_ATTRIBUTE& accountInfo, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Background_Account_Info"))
			return false;

		TiXmlElement* pAccountInfoElement = dataElement.FirstChildElement("AccountInfo");
		if (pAccountInfoElement == NULL)
			return false;

		if (pAccountInfoElement->Attribute("emType") != NULL)
			accountInfo.emType = (EM_BACKSTAGE_TYPE)atoi(pAccountInfoElement->Attribute("emType"));

		if (pAccountInfoElement->Attribute("type") != NULL)
			accountInfo.strType = CStringUtil::Utf8ToTStr(pAccountInfoElement->Attribute("type"));

		if (pAccountInfoElement->Attribute("subType") != NULL)
			accountInfo.strSubType = CStringUtil::Utf8ToTStr(pAccountInfoElement->Attribute("subType"));

		if (pAccountInfoElement->Attribute("userName") != NULL)
			accountInfo.strUserName = CStringUtil::Utf8ToTStr(pAccountInfoElement->Attribute("userName"));

		if (pAccountInfoElement->Attribute("password") != NULL)
			accountInfo.strPassword = CStringUtil::Utf8ToTStr(pAccountInfoElement->Attribute("password"));

		return true;
	}

	static void BuildUpdateBackgroundAccountInfoBs(const BACKSTAGE_ACCOUNT_ATTRIBUTE& accountInfo, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement accountInfoElement("AccountInfo");

		accountInfoElement.SetAttribute("emType", accountInfo.emType);
		accountInfoElement.SetAttribute("id", accountInfo.nId);
		accountInfoElement.SetAttribute("userName", CStringUtil::TStrToUtf8(accountInfo.strUserName).c_str());
		accountInfoElement.SetAttribute("password", CStringUtil::TStrToUtf8(accountInfo.strPassword).c_str());

		dataElement.InsertEndChild(accountInfoElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Background_Account_Info"), dataElement), outByteStream);
	}

	static bool ParseUpdateBackgroundAccountInfoBs(BACKSTAGE_ACCOUNT_ATTRIBUTE& accountInfo, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Background_Account_Info"))
			return false;

		TiXmlElement* pAccountInfoElement = dataElement.FirstChildElement("AccountInfo");
		if (pAccountInfoElement == NULL)
			return false;

		if (pAccountInfoElement->Attribute("emType") != NULL)
			accountInfo.emType = (EM_BACKSTAGE_TYPE)atoi(pAccountInfoElement->Attribute("emType"));

		if (pAccountInfoElement->Attribute("id") != NULL)
			accountInfo.nId = atoi(pAccountInfoElement->Attribute("id"));

		if (pAccountInfoElement->Attribute("userName") != NULL)
			accountInfo.strUserName = CStringUtil::Utf8ToTStr(pAccountInfoElement->Attribute("userName"));

		if (pAccountInfoElement->Attribute("password") != NULL)
			accountInfo.strPassword = CStringUtil::Utf8ToTStr(pAccountInfoElement->Attribute("password"));

		return true;
	}

	static void BuildDeleteBackgroundAccountInfoBs(const BACKSTAGE_ACCOUNT_ATTRIBUTE& accountInfo, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		
		TiXmlElement accountInfoElement("AccountInfo");
		accountInfoElement.SetAttribute("emType", accountInfo.emType);
		accountInfoElement.SetAttribute("id", accountInfo.nId);
		accountInfoElement.SetAttribute("userName", CStringUtil::TStrToUtf8(accountInfo.strUserName).c_str());

		dataElement.InsertEndChild(accountInfoElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Background_Account_Info"), dataElement), outByteStream);
	}

	static bool ParseDeleteBackgroundAccountInfoBs(BACKSTAGE_ACCOUNT_ATTRIBUTE& accountInfo, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Background_Account_Info"))
			return false;

		TiXmlElement* pAccountInfoElement = dataElement.FirstChildElement("AccountInfo");
		if (pAccountInfoElement == NULL)
			return false;

		if (pAccountInfoElement->Attribute("emType") != NULL)
			accountInfo.emType = (EM_BACKSTAGE_TYPE)atoi(pAccountInfoElement->Attribute("emType"));

		if (pAccountInfoElement->Attribute("id") != NULL)
			accountInfo.nId = atoi(pAccountInfoElement->Attribute("id"));

		if (pAccountInfoElement->Attribute("userName") != NULL)
			accountInfo.strUserName = CStringUtil::Utf8ToTStr(pAccountInfoElement->Attribute("userName"));

		return true;
	}

	static void BuildGetBackStageTypeInfoBs(CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");

		BuildXmlBs(BuildProtoXmlString(_T("Get_Backstage_Type_Info"), dataElement), outByteStream);
	}

	static bool ParseGetBackStageTypeInfoBs(CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Backstage_Type_Info"))
			return false;

		return true;
	}

	static void BuildAddBackStageTypeInfoBs(const BACKSTAGE_ACCOUNT_ATTRIBUTE& typeInfo, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement typeInfoElement("TypeInfo");

		typeInfoElement.SetAttribute("emType", typeInfo.emType);
		typeInfoElement.SetAttribute("type", CStringUtil::TStrToUtf8(typeInfo.strType).c_str());
		typeInfoElement.SetAttribute("subType", CStringUtil::TStrToUtf8(typeInfo.strSubType).c_str());
		typeInfoElement.SetAttribute("url", CStringUtil::TStrToUtf8(typeInfo.strUrl).c_str());
		typeInfoElement.SetAttribute("script", CStringUtil::TStrToUtf8(typeInfo.strScript).c_str());

		dataElement.InsertEndChild(typeInfoElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Backstage_Type_Info"), dataElement), outByteStream);
	}

	static bool ParseAddBackStageTypeInfoBs(BACKSTAGE_ACCOUNT_ATTRIBUTE& typeInfo, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Backstage_Type_Info"))
			return false;

		TiXmlElement* pTypeInfoElement = dataElement.FirstChildElement("TypeInfo");
		if (pTypeInfoElement == NULL)
			return false;

		if (pTypeInfoElement->Attribute("emType") != NULL)
			typeInfo.emType = (EM_BACKSTAGE_TYPE)atoi(pTypeInfoElement->Attribute("emType"));

		if (pTypeInfoElement->Attribute("type") != NULL)
			typeInfo.strType = CStringUtil::Utf8ToTStr(pTypeInfoElement->Attribute("type"));

		if (pTypeInfoElement->Attribute("subType") != NULL)
			typeInfo.strSubType = CStringUtil::Utf8ToTStr(pTypeInfoElement->Attribute("subType"));

		if (pTypeInfoElement->Attribute("url") != NULL)
			typeInfo.strUrl = CStringUtil::Utf8ToTStr(pTypeInfoElement->Attribute("url"));

		if (pTypeInfoElement->Attribute("script") != NULL)
			typeInfo.strScript = CStringUtil::Utf8ToTStr(pTypeInfoElement->Attribute("script"));

		return true;
	}

	static void BuildUpdateBackStageTypeInfoBs(const BACKSTAGE_ACCOUNT_ATTRIBUTE& typeInfo, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement typeInfoElement("TypeInfo");

		typeInfoElement.SetAttribute("id", typeInfo.nId);
		typeInfoElement.SetAttribute("emType", typeInfo.emType);
		typeInfoElement.SetAttribute("url", CStringUtil::TStrToUtf8(typeInfo.strUrl).c_str());
		typeInfoElement.SetAttribute("script", CStringUtil::TStrToUtf8(typeInfo.strScript).c_str());

		dataElement.InsertEndChild(typeInfoElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Backstage_Type_Info"), dataElement), outByteStream);
	}

	static bool ParseUpdateBackStageTypeInfoBs(BACKSTAGE_ACCOUNT_ATTRIBUTE& typeInfo, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Backstage_Type_Info"))
			return false;

		TiXmlElement* pTypeInfoElement = dataElement.FirstChildElement("TypeInfo");
		if (pTypeInfoElement == NULL)
			return false;

		if (pTypeInfoElement->Attribute("id") != NULL)
			typeInfo.nId = atoi(pTypeInfoElement->Attribute("id"));

		if (pTypeInfoElement->Attribute("emType") != NULL)
			typeInfo.emType = (EM_BACKSTAGE_TYPE)atoi(pTypeInfoElement->Attribute("emType"));

		if (pTypeInfoElement->Attribute("url") != NULL)
			typeInfo.strUrl = CStringUtil::Utf8ToTStr(pTypeInfoElement->Attribute("url"));

		if (pTypeInfoElement->Attribute("script") != NULL)
			typeInfo.strScript = CStringUtil::Utf8ToTStr(pTypeInfoElement->Attribute("script"));

		return true;
	}

	static void BuildDeleteBackStageTypeInfoBs(const BACKSTAGE_ACCOUNT_ATTRIBUTE& typeInfo, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement typeInfoElement("TypeInfo");

		typeInfoElement.SetAttribute("id", typeInfo.nId);
		typeInfoElement.SetAttribute("emType", typeInfo.emType);

		dataElement.InsertEndChild(typeInfoElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Backstage_Type_Info"), dataElement), outByteStream);
	}

	static bool ParseDeleteBackStageTypeInfoBs(BACKSTAGE_ACCOUNT_ATTRIBUTE& typeInfo, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Backstage_Type_Info"))
			return false;

		TiXmlElement* pTypeInfoElement = dataElement.FirstChildElement("TypeInfo");
		if (pTypeInfoElement == NULL)
			return false;

		if (pTypeInfoElement->Attribute("id") != NULL)
			typeInfo.nId = atoi(pTypeInfoElement->Attribute("id"));
		if (pTypeInfoElement->Attribute("emType") != NULL)
			typeInfo.emType = (EM_BACKSTAGE_TYPE)atoi(pTypeInfoElement->Attribute("emType"));

		return true;
	}

	//////////////////////////////////////////服务端发送&&相应解析///////////////////////////////////////////
	static void BuildCheckUpdateAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Check_Update_Ack"), dataElement), outByteStream);
	}

	static bool ParseCheckUpdateAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Check_Update_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildUserLoginAckBs(int nResult, const tstring& strUserName, const tstring& strRoleName, const tstring& strFtpUserName, const tstring& strFtpPassword, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement userNameElement("UserName");
		TiXmlElement roleNameElement("RoleName");
		TiXmlElement ftpUserNameElement("FtpUserName");
		TiXmlElement ftpPasswordElement("FtpPassword");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		InsertTextToEndChild(userNameElement, _T("%s"), strUserName.c_str());
		InsertTextToEndChild(roleNameElement, _T("%s"), strRoleName.c_str());
		InsertTextToEndChild(ftpUserNameElement, _T("%s"), strFtpUserName.c_str());
		InsertTextToEndChild(ftpPasswordElement, _T("%s"), strFtpPassword.c_str());
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(userNameElement);
		dataElement.InsertEndChild(roleNameElement);
		dataElement.InsertEndChild(ftpUserNameElement);
		dataElement.InsertEndChild(ftpPasswordElement);

		BuildXmlBs(BuildProtoXmlString(_T("User_Login_Ack"), dataElement), outByteStream);
	}

	static bool ParseBuildUserLoginAckBs(int& nResult, tstring& strUserName, tstring& strRoleName, tstring& strFtpUserName, tstring& strFtpPassword, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("User_Login_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pUserNameElement = dataElement.FirstChildElement("UserName");
		if (pUserNameElement == NULL)
			return false;

		TiXmlElement* pRoleNameElement = dataElement.FirstChildElement("RoleName");
		if (pRoleNameElement == NULL)
			return false;

		TiXmlElement* pFtpUserNameElement = dataElement.FirstChildElement("FtpUserName");
		if (pFtpUserNameElement == NULL)
			return false;

		TiXmlElement* pFtpPasswordElement = dataElement.FirstChildElement("FtpPassword");
		if (pFtpPasswordElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		if (pUserNameElement->GetText() != NULL)
			strUserName = CStringUtil::Utf8ToTStr(pUserNameElement->GetText());

		if (pRoleNameElement->GetText() != NULL)
			strRoleName = CStringUtil::Utf8ToTStr(pRoleNameElement->GetText());

		if (pFtpUserNameElement->GetText() != NULL)
			strFtpUserName = CStringUtil::Utf8ToTStr(pFtpUserNameElement->GetText());

		if (pFtpPasswordElement->GetText() != NULL)
			strFtpPassword = CStringUtil::Utf8ToTStr(pFtpPasswordElement->GetText());

		return true;
	}

	static void BuildSelectRechargeOrderUseAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Select_RechargeOrder_Use_Ack"), dataElement), outByteStream);
	}

	static bool ParseSelectRechargeOrderUseAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Select_RechargeOrder_Use_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildGetGameAccountAckBs(int nResult, const std::vector<CGameAccount>& vecGameAccount, int nTotalCount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement gameAccountListElement("GameAccountList");
		for (int i = 0; i < vecGameAccount.size(); i++)
		{
			TiXmlElement gameAccountElement("GameAccount");
			gameAccountElement.SetAttribute("id", vecGameAccount[i].m_nId);
			gameAccountElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecGameAccount[i].m_strUserName).c_str());
			gameAccountElement.SetAttribute("password", CStringUtil::TStrToUtf8(vecGameAccount[i].m_strPassword).c_str());
			gameAccountElement.SetAttribute("gameName", CStringUtil::TStrToUtf8(vecGameAccount[i].m_strGameName).c_str());
			gameAccountListElement.InsertEndChild(gameAccountElement);
		}
		TiXmlElement totalCountElement("TotalCount");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		InsertTextToEndChild(totalCountElement, _T("%d"), nTotalCount);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(gameAccountListElement);
		dataElement.InsertEndChild(totalCountElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_GameAccount_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetGameAccountAckBs(int& nResult, std::vector<CGameAccount>& vecGameAccount, int& nTotalCount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_GameAccount_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pGameAccountListElement = dataElement.FirstChildElement("GameAccountList");
		if (pGameAccountListElement == NULL)
			return false;

		TiXmlElement* pTotalCountElement = dataElement.FirstChildElement("TotalCount");
		if (pTotalCountElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		if (pTotalCountElement->GetText() != NULL)
			nTotalCount = atoi(pTotalCountElement->GetText());

		TiXmlElement* pGameAccountElement = NULL;
		for (pGameAccountElement = pGameAccountListElement->FirstChildElement("GameAccount"); pGameAccountElement; pGameAccountElement = pGameAccountElement->NextSiblingElement("GameAccount"))
		{
			CGameAccount gameAccount;
			if (pGameAccountElement->Attribute("id") != NULL)
				gameAccount.m_nId = atoi(pGameAccountElement->Attribute("id"));
			if (pGameAccountElement->Attribute("userName") != NULL)
				gameAccount.m_strUserName = CStringUtil::Utf8ToTStr(pGameAccountElement->Attribute("userName"));
			if (pGameAccountElement->Attribute("password") != NULL)
				gameAccount.m_strPassword = CStringUtil::Utf8ToTStr(pGameAccountElement->Attribute("password"));
			if (pGameAccountElement->Attribute("gameName") != NULL)
				gameAccount.m_strGameName = CStringUtil::Utf8ToTStr(pGameAccountElement->Attribute("gameName"));
			vecGameAccount.push_back(gameAccount);
		}

		return true;
	}

	static void BuildSelectGameAccountUseAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Select_GameAccount_Use_Ack"), dataElement), outByteStream);
	}

	static bool ParseSelectGameAccountUseAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Select_GameAccount_Use_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildAddRechargePayOrderAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_RechargePayOrder_Ack"), dataElement), outByteStream);
	}

	static bool ParseAddRechargePayOrderAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_RechargePayOrder_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildGetRechargeOrderAckBs(int nResult, const std::vector<CRechargeOrder>& vecRechargeOrder, int nTotalCount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement rechargeOrderListElement("RechargeOrderList");
		for (int i = 0; i < vecRechargeOrder.size(); i++)
		{
			TiXmlElement rechargeOrderElement("RechargeOrder");
			rechargeOrderElement.SetAttribute("id", vecRechargeOrder[i].m_nId);
			rechargeOrderElement.SetAttribute("type", vecRechargeOrder[i].m_nType);
			rechargeOrderElement.SetAttribute("number", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strNumber).c_str());
			rechargeOrderElement.SetAttribute("customerId", vecRechargeOrder[i].m_nCustomerId);
			rechargeOrderElement.SetAttribute("productName", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strProductName).c_str());
			rechargeOrderElement.SetDoubleAttribute("price", vecRechargeOrder[i].m_fPrice);
			rechargeOrderElement.SetDoubleAttribute("payPrice", vecRechargeOrder[i].m_fPayPrice);
			rechargeOrderElement.SetAttribute("qQNumber", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strQQNumber).c_str());
			rechargeOrderElement.SetAttribute("telephoneNumber", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strTelephoneNumber).c_str());
			rechargeOrderElement.SetAttribute("roleName", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strRoleName).c_str());
			rechargeOrderElement.SetAttribute("standbyRoleName", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strStandbyRoleName).c_str());
			rechargeOrderElement.SetAttribute("roleAttri", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strRoleAttri).c_str());
			rechargeOrderElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strUserName).c_str());
			rechargeOrderElement.SetAttribute("password", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strPassword).c_str());
			rechargeOrderElement.SetAttribute("gameName", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strGameName).c_str());
			rechargeOrderElement.SetAttribute("serviceArea", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strServiceArea).c_str());
			rechargeOrderElement.SetAttribute("channel", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strChannel).c_str());
			rechargeOrderElement.SetAttribute("state", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strState).c_str());
			rechargeOrderElement.SetAttribute("buyTime", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strBuyTime).c_str());
			rechargeOrderElement.SetAttribute("payChannel", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strPayChannel).c_str());
			rechargeOrderElement.SetAttribute("payChannelOrderNumber", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strPayChannelOrderNumber).c_str());
			rechargeOrderElement.SetAttribute("bankOrderNumber", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strBankOrderNumber).c_str());
			rechargeOrderElement.SetAttribute("payer", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strPayer).c_str());
			rechargeOrderElement.SetAttribute("deliveryNote", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strDeliveryNote).c_str());
			rechargeOrderElement.SetAttribute("payNote", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strPayNote).c_str());
			rechargeOrderElement.SetAttribute("gifeCode", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_strGifeCode).c_str());
			rechargeOrderElement.SetDoubleAttribute("rebateRatio", vecRechargeOrder[i].m_fRebateRatio);

			TiXmlElement snapshotListElement("SnapshotList");
			for (int j = 0; j < vecRechargeOrder[i].m_vecSnapshot.size(); j++)
			{
				TiXmlElement snapshotElement("Snapshot");
				snapshotElement.SetAttribute("type", vecRechargeOrder[i].m_vecSnapshot[j].m_nType);
				snapshotElement.SetAttribute("hashFileName", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_vecSnapshot[j].m_strHashFileName).c_str());
				snapshotElement.SetAttribute("originalFileName", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_vecSnapshot[j].m_strOriginalFileName).c_str());
				snapshotElement.SetAttribute("originalFilePath", CStringUtil::TStrToUtf8(vecRechargeOrder[i].m_vecSnapshot[j].m_strOriginalFilePath).c_str());

				snapshotListElement.InsertEndChild(snapshotElement);
			}
			rechargeOrderElement.InsertEndChild(snapshotListElement);

			TiXmlElement platformOrderNumberListElement("PlatformOrderNumberList");
			for (int j = 0; j < vecRechargeOrder[i].m_vecPlatformOrderNumber.size(); j++)
			{
				TiXmlElement platformOrderNumberElement("PlatformOrderNumber");
				InsertTextToEndChild(platformOrderNumberElement, _T("%s"), vecRechargeOrder[i].m_vecPlatformOrderNumber[j].c_str());

				platformOrderNumberListElement.InsertEndChild(platformOrderNumberElement);
			}
			rechargeOrderElement.InsertEndChild(platformOrderNumberListElement);

			rechargeOrderListElement.InsertEndChild(rechargeOrderElement);
		}
		TiXmlElement totalCountElement("TotalCount");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		InsertTextToEndChild(totalCountElement, _T("%d"), nTotalCount);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(rechargeOrderListElement);
		dataElement.InsertEndChild(totalCountElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_RechargeOrder_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetRechargeOrderAckBs(int& nResult, std::vector<CRechargeOrder>& vecRechargeOrder, int& nTotalCount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_RechargeOrder_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pRechargeOrderListElement = dataElement.FirstChildElement("RechargeOrderList");
		if (pRechargeOrderListElement == NULL)
			return false;

		TiXmlElement* pTotalCountElement = dataElement.FirstChildElement("TotalCount");
		if (pTotalCountElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		if (pTotalCountElement->GetText() != NULL)
			nTotalCount = atoi(pTotalCountElement->GetText());

		TiXmlElement* pRechargeOrderElement = NULL;
		for (pRechargeOrderElement = pRechargeOrderListElement->FirstChildElement("RechargeOrder"); pRechargeOrderElement; pRechargeOrderElement = pRechargeOrderElement->NextSiblingElement("RechargeOrder"))
		{
			CRechargeOrder rechargeOrder;
			if (pRechargeOrderElement->Attribute("id") != NULL)
				rechargeOrder.m_nId = atoi(pRechargeOrderElement->Attribute("id"));
			if (pRechargeOrderElement->Attribute("type") != NULL)
				rechargeOrder.m_nType = atoi(pRechargeOrderElement->Attribute("type"));
			if (pRechargeOrderElement->Attribute("number") != NULL)
				rechargeOrder.m_strNumber = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("number"));
			if (pRechargeOrderElement->Attribute("customerId") != NULL)
				rechargeOrder.m_nCustomerId = atoi(pRechargeOrderElement->Attribute("customerId"));
			if (pRechargeOrderElement->Attribute("productName") != NULL)
				rechargeOrder.m_strProductName = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("productName"));
			if (pRechargeOrderElement->Attribute("price") != NULL)
				rechargeOrder.m_fPrice = atof(pRechargeOrderElement->Attribute("price"));
			if (pRechargeOrderElement->Attribute("payPrice") != NULL)
				rechargeOrder.m_fPayPrice = atof(pRechargeOrderElement->Attribute("payPrice"));
			if (pRechargeOrderElement->Attribute("qQNumber") != NULL)
				rechargeOrder.m_strQQNumber = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("qQNumber"));
			if (pRechargeOrderElement->Attribute("telephoneNumber") != NULL)
				rechargeOrder.m_strTelephoneNumber = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("telephoneNumber"));
			if (pRechargeOrderElement->Attribute("roleName") != NULL)
				rechargeOrder.m_strRoleName = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("roleName"));
			if (pRechargeOrderElement->Attribute("standbyRoleName") != NULL)
				rechargeOrder.m_strStandbyRoleName = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("standbyRoleName"));
			if (pRechargeOrderElement->Attribute("roleAttri") != NULL)
				rechargeOrder.m_strRoleAttri = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("roleAttri"));
			if (pRechargeOrderElement->Attribute("userName") != NULL)
				rechargeOrder.m_strUserName = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("userName"));
			if (pRechargeOrderElement->Attribute("password") != NULL)
				rechargeOrder.m_strPassword = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("password"));
			if (pRechargeOrderElement->Attribute("gameName") != NULL)
				rechargeOrder.m_strGameName = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("gameName"));
			if (pRechargeOrderElement->Attribute("serviceArea") != NULL)
				rechargeOrder.m_strServiceArea = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("serviceArea"));
			if (pRechargeOrderElement->Attribute("channel") != NULL)
				rechargeOrder.m_strChannel = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("channel"));
			if (pRechargeOrderElement->Attribute("state") != NULL)
				rechargeOrder.m_strState = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("state"));
			if (pRechargeOrderElement->Attribute("buyTime") != NULL)
				rechargeOrder.m_strBuyTime = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("buyTime"));
			if (pRechargeOrderElement->Attribute("payChannel") != NULL)
				rechargeOrder.m_strPayChannel = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("payChannel"));
			if (pRechargeOrderElement->Attribute("payChannelOrderNumber") != NULL)
				rechargeOrder.m_strPayChannelOrderNumber = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("payChannelOrderNumber"));
			if (pRechargeOrderElement->Attribute("bankOrderNumber") != NULL)
				rechargeOrder.m_strBankOrderNumber = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("bankOrderNumber"));
			if (pRechargeOrderElement->Attribute("payer") != NULL)
				rechargeOrder.m_strPayer = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("payer"));
			if (pRechargeOrderElement->Attribute("deliveryNote") != NULL)
				rechargeOrder.m_strDeliveryNote = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("deliveryNote"));
			if (pRechargeOrderElement->Attribute("payNote") != NULL)
				rechargeOrder.m_strPayNote = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("payNote"));
			if (pRechargeOrderElement->Attribute("gifeCode") != NULL)
				rechargeOrder.m_strGifeCode = CStringUtil::Utf8ToTStr(pRechargeOrderElement->Attribute("gifeCode"));
			if (pRechargeOrderElement->Attribute("rebateRatio") != NULL)
				rechargeOrder.m_fRebateRatio = atof(pRechargeOrderElement->Attribute("rebateRatio"));

			TiXmlElement* pSnapshotListElement = pRechargeOrderElement->FirstChildElement("SnapshotList");
			if (pSnapshotListElement != NULL)
			{
				TiXmlElement* pSnapshotElement = NULL;
				for (pSnapshotElement = pSnapshotListElement->FirstChildElement("Snapshot"); pSnapshotElement; pSnapshotElement = pSnapshotElement->NextSiblingElement("Snapshot"))
				{
					COrderSnapshot orderSnapshot;
					if (pSnapshotElement->Attribute("type") != NULL)
						orderSnapshot.m_nType = atoi(pSnapshotElement->Attribute("type"));
					if (pSnapshotElement->Attribute("hashFileName") != NULL)
						orderSnapshot.m_strHashFileName = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("hashFileName"));
					if (pSnapshotElement->Attribute("originalFileName"))
						orderSnapshot.m_strOriginalFileName = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("originalFileName"));
					if (pSnapshotElement->Attribute("originalFilePath"))
						orderSnapshot.m_strOriginalFilePath = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("originalFilePath"));

					rechargeOrder.m_vecSnapshot.push_back(orderSnapshot);
				}
			}

			TiXmlElement* pPlatformOrderNumberListElement = pRechargeOrderElement->FirstChildElement("PlatformOrderNumberList");
			if (pPlatformOrderNumberListElement != NULL)
			{
				TiXmlElement* pPlatformOrderNumberElement = NULL;
				for (pPlatformOrderNumberElement = pPlatformOrderNumberListElement->FirstChildElement("PlatformOrderNumber"); pPlatformOrderNumberElement; pPlatformOrderNumberElement = pPlatformOrderNumberElement->NextSiblingElement("PlatformOrderNumber"))
				{
					if (pPlatformOrderNumberElement->GetText() != NULL)
						rechargeOrder.m_vecPlatformOrderNumber.push_back(CStringUtil::Utf8ToTStr(pPlatformOrderNumberElement->GetText()));
				}
			}

			vecRechargeOrder.push_back(rechargeOrder);
		}

		return true;
	}

	static void BuildUpdateRechargeOrderAckBs(int nResult, int nOrderId, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement orderIdElement("OrderId");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		InsertTextToEndChild(orderIdElement, _T("%d"), nOrderId);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(orderIdElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_RechargeOrder_Ack"), dataElement), outByteStream);
	}

	static bool ParseUpdateRechargeOrderAckBs(int& nResult, int& nOrderId, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_RechargeOrder_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pOrderIdElement = dataElement.FirstChildElement("OrderId");
		if (pOrderIdElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		if (pOrderIdElement->GetText() != NULL)
			nOrderId = atoi(pOrderIdElement->GetText());

		return true;
	}

	static void BuildGetRechargeOrderExportAckBs(int nResult, const std::vector<CRechargeOrderPayEx>& vecRechargeOrderEx, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement rechargeOrderPayExListElement("RechargeOrderPayExList");
		for (int i = 0; i < vecRechargeOrderEx.size(); i++)
		{
			TiXmlElement rechargeOrderPayExElement("RechargeOrderPayEx");
			rechargeOrderPayExElement.SetAttribute("id", vecRechargeOrderEx[i].m_nId);
			rechargeOrderPayExElement.SetAttribute("type", vecRechargeOrderEx[i].m_nType);
			rechargeOrderPayExElement.SetAttribute("number", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strNumber).c_str());
			rechargeOrderPayExElement.SetAttribute("customerId", vecRechargeOrderEx[i].m_nCustomerId);
			rechargeOrderPayExElement.SetAttribute("productName", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strProductName).c_str());
			rechargeOrderPayExElement.SetDoubleAttribute("price", vecRechargeOrderEx[i].m_fPrice);
			rechargeOrderPayExElement.SetDoubleAttribute("payPrice", vecRechargeOrderEx[i].m_fPayPrice);
			rechargeOrderPayExElement.SetAttribute("qQNumber", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strQQNumber).c_str());
			rechargeOrderPayExElement.SetAttribute("telephoneNumber", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strTelephoneNumber).c_str());
			rechargeOrderPayExElement.SetAttribute("roleName", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strRoleName).c_str());
			rechargeOrderPayExElement.SetAttribute("standbyRoleName", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strStandbyRoleName).c_str());
			rechargeOrderPayExElement.SetAttribute("roleAttri", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strRoleAttri).c_str());
			rechargeOrderPayExElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strUserName).c_str());
			rechargeOrderPayExElement.SetAttribute("password", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strPassword).c_str());
			rechargeOrderPayExElement.SetAttribute("gameName", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strGameName).c_str());
			rechargeOrderPayExElement.SetAttribute("serviceArea", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strServiceArea).c_str());
			rechargeOrderPayExElement.SetAttribute("channel", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strChannel).c_str());
			rechargeOrderPayExElement.SetAttribute("state", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strState).c_str());
			rechargeOrderPayExElement.SetAttribute("buyTime", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strBuyTime).c_str());
			rechargeOrderPayExElement.SetAttribute("payChannel", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strPayChannel).c_str());
			rechargeOrderPayExElement.SetAttribute("payChannelOrderNumber", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strPayChannelOrderNumber).c_str());
			rechargeOrderPayExElement.SetAttribute("bankOrderNumber", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strBankOrderNumber).c_str());
			rechargeOrderPayExElement.SetAttribute("payer", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strPayer).c_str());
			rechargeOrderPayExElement.SetAttribute("deliveryNote", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strDeliveryNote).c_str());
			rechargeOrderPayExElement.SetAttribute("payNote", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strPayNote).c_str());
			rechargeOrderPayExElement.SetAttribute("gifeCode", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_strGifeCode).c_str());
			rechargeOrderPayExElement.SetDoubleAttribute("rebateRatio", vecRechargeOrderEx[i].m_fRebateRatio);

			TiXmlElement snapshotListElement("SnapshotList");
			for (int j = 0; j < vecRechargeOrderEx[i].m_vecSnapshot.size(); j++)
			{
				TiXmlElement snapshotElement("Snapshot");
				snapshotElement.SetAttribute("type", vecRechargeOrderEx[i].m_vecSnapshot[j].m_nType);
				snapshotElement.SetAttribute("hashFileName", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_vecSnapshot[j].m_strHashFileName).c_str());
				snapshotElement.SetAttribute("originalFileName", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_vecSnapshot[j].m_strOriginalFileName).c_str());
				snapshotElement.SetAttribute("originalFilePath", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_vecSnapshot[j].m_strOriginalFilePath).c_str());

				snapshotListElement.InsertEndChild(snapshotElement);
			}
			rechargeOrderPayExElement.InsertEndChild(snapshotListElement);

			TiXmlElement platformOrderNumberListElement("PlatformOrderNumberList");
			for (int j = 0; j < vecRechargeOrderEx[i].m_vecPlatformOrderNumber.size(); j++)
			{
				TiXmlElement platformOrderNumberElement("PlatformOrderNumber");
				InsertTextToEndChild(platformOrderNumberElement, _T("%s"), vecRechargeOrderEx[i].m_vecPlatformOrderNumber[j].c_str());

				platformOrderNumberListElement.InsertEndChild(platformOrderNumberElement);
			}
			rechargeOrderPayExElement.InsertEndChild(platformOrderNumberListElement);

			TiXmlElement payOrderListElement("PayOrderList");
			for (int j = 0; j < vecRechargeOrderEx[i].m_vecPayOrder.size(); j++)
			{
				TiXmlElement payOrderElement("PayOrder");
				payOrderElement.SetAttribute("orderNumber", CStringUtil::TStrToUtf8(vecRechargeOrderEx[i].m_vecPayOrder[j].m_nOrderId).c_str());
				payOrderElement.SetDoubleAttribute("buyPrice", vecRechargeOrderEx[i].m_vecPayOrder[j].m_fBuyPrice);

				payOrderListElement.InsertEndChild(payOrderElement);
			}
			rechargeOrderPayExElement.InsertEndChild(payOrderListElement);

			rechargeOrderPayExListElement.InsertEndChild(rechargeOrderPayExElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(rechargeOrderPayExListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_RechargeOrder_Export_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetRechargeOrderExportAckBs(int& nResult, std::vector<CRechargeOrderPayEx>& vecRechargeOrderEx, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_RechargeOrder_Export_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pRechargeOrderPayExListElement = dataElement.FirstChildElement("RechargeOrderPayExList");
		if (pRechargeOrderPayExListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pRechargeOrderPayExElement = NULL;
		for (pRechargeOrderPayExElement = pRechargeOrderPayExListElement->FirstChildElement("RechargeOrderPayEx"); pRechargeOrderPayExElement; pRechargeOrderPayExElement = pRechargeOrderPayExElement->NextSiblingElement("RechargeOrderPayEx"))
		{
			CRechargeOrderPayEx rechargeOrderPayEx;
			if (pRechargeOrderPayExElement->Attribute("id") != NULL)
				rechargeOrderPayEx.m_nId = atoi(pRechargeOrderPayExElement->Attribute("id"));
			if (pRechargeOrderPayExElement->Attribute("type") != NULL)
				rechargeOrderPayEx.m_nType = atoi(pRechargeOrderPayExElement->Attribute("type"));
			if (pRechargeOrderPayExElement->Attribute("number") != NULL)
				rechargeOrderPayEx.m_strNumber = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("number"));
			if (pRechargeOrderPayExElement->Attribute("customerId") != NULL)
				rechargeOrderPayEx.m_nCustomerId = atoi(pRechargeOrderPayExElement->Attribute("customerId"));
			if (pRechargeOrderPayExElement->Attribute("productName") != NULL)
				rechargeOrderPayEx.m_strProductName = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("productName"));
			if (pRechargeOrderPayExElement->Attribute("price") != NULL)
				rechargeOrderPayEx.m_fPrice = atof(pRechargeOrderPayExElement->Attribute("price"));
			if (pRechargeOrderPayExElement->Attribute("payPrice") != NULL)
				rechargeOrderPayEx.m_fPayPrice = atof(pRechargeOrderPayExElement->Attribute("payPrice"));
			if (pRechargeOrderPayExElement->Attribute("qQNumber") != NULL)
				rechargeOrderPayEx.m_strQQNumber = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("qQNumber"));
			if (pRechargeOrderPayExElement->Attribute("telephoneNumber") != NULL)
				rechargeOrderPayEx.m_strTelephoneNumber = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("telephoneNumber"));
			if (pRechargeOrderPayExElement->Attribute("roleName") != NULL)
				rechargeOrderPayEx.m_strRoleName = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("roleName"));
			if (pRechargeOrderPayExElement->Attribute("standbyRoleName") != NULL)
				rechargeOrderPayEx.m_strStandbyRoleName = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("standbyRoleName"));
			if (pRechargeOrderPayExElement->Attribute("roleAttri") != NULL)
				rechargeOrderPayEx.m_strRoleAttri = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("roleAttri"));
			if (pRechargeOrderPayExElement->Attribute("userName") != NULL)
				rechargeOrderPayEx.m_strUserName = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("userName"));
			if (pRechargeOrderPayExElement->Attribute("password") != NULL)
				rechargeOrderPayEx.m_strPassword = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("password"));
			if (pRechargeOrderPayExElement->Attribute("gameName") != NULL)
				rechargeOrderPayEx.m_strGameName = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("gameName"));
			if (pRechargeOrderPayExElement->Attribute("serviceArea") != NULL)
				rechargeOrderPayEx.m_strServiceArea = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("serviceArea"));
			if (pRechargeOrderPayExElement->Attribute("channel") != NULL)
				rechargeOrderPayEx.m_strChannel = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("channel"));
			if (pRechargeOrderPayExElement->Attribute("state") != NULL)
				rechargeOrderPayEx.m_strState = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("state"));
			if (pRechargeOrderPayExElement->Attribute("buyTime") != NULL)
				rechargeOrderPayEx.m_strBuyTime = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("buyTime"));
			if (pRechargeOrderPayExElement->Attribute("payChannel") != NULL)
				rechargeOrderPayEx.m_strPayChannel = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("payChannel"));
			if (pRechargeOrderPayExElement->Attribute("payChannelOrderNumber") != NULL)
				rechargeOrderPayEx.m_strPayChannelOrderNumber = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("payChannelOrderNumber"));
			if (pRechargeOrderPayExElement->Attribute("bankOrderNumber") != NULL)
				rechargeOrderPayEx.m_strBankOrderNumber = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("bankOrderNumber"));
			if (pRechargeOrderPayExElement->Attribute("payer") != NULL)
				rechargeOrderPayEx.m_strPayer = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("payer"));
			if (pRechargeOrderPayExElement->Attribute("deliveryNote") != NULL)
				rechargeOrderPayEx.m_strDeliveryNote = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("deliveryNote"));
			if (pRechargeOrderPayExElement->Attribute("payNote") != NULL)
				rechargeOrderPayEx.m_strPayNote = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("payNote"));
			if (pRechargeOrderPayExElement->Attribute("gifeCode") != NULL)
				rechargeOrderPayEx.m_strGifeCode = CStringUtil::Utf8ToTStr(pRechargeOrderPayExElement->Attribute("gifeCode"));
			if (pRechargeOrderPayExElement->Attribute("rebateRatio") != NULL)
				rechargeOrderPayEx.m_fRebateRatio = atof(pRechargeOrderPayExElement->Attribute("rebateRatio"));

			TiXmlElement* pSnapshotListElement = pRechargeOrderPayExElement->FirstChildElement("SnapshotList");
			if (pSnapshotListElement != NULL)
			{
				TiXmlElement* pSnapshotElement = NULL;
				for (pSnapshotElement = pSnapshotListElement->FirstChildElement("Snapshot"); pSnapshotElement; pSnapshotElement = pSnapshotElement->NextSiblingElement("Snapshot"))
				{
					COrderSnapshot orderSnapshot;
					if (pSnapshotElement->Attribute("type") != NULL)
						orderSnapshot.m_nType = atoi(pSnapshotElement->Attribute("type"));
					if (pSnapshotElement->Attribute("hashFileName") != NULL)
						orderSnapshot.m_strHashFileName = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("hashFileName"));
					if (pSnapshotElement->Attribute("originalFileName"))
						orderSnapshot.m_strOriginalFileName = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("originalFileName"));
					if (pSnapshotElement->Attribute("originalFilePath"))
						orderSnapshot.m_strOriginalFilePath = CStringUtil::Utf8ToTStr(pSnapshotElement->Attribute("originalFilePath"));

					rechargeOrderPayEx.m_vecSnapshot.push_back(orderSnapshot);
				}
			}

			TiXmlElement* pPlatformOrderNumberListElement = pRechargeOrderPayExElement->FirstChildElement("PlatformOrderNumberList");
			if (pPlatformOrderNumberListElement != NULL)
			{
				TiXmlElement* pPlatformOrderNumberElement = NULL;
				for (pPlatformOrderNumberElement = pPlatformOrderNumberListElement->FirstChildElement("PlatformOrderNumber"); pPlatformOrderNumberElement; pPlatformOrderNumberElement = pPlatformOrderNumberElement->NextSiblingElement("PlatformOrderNumber"))
				{
					if (pPlatformOrderNumberElement->GetText() != NULL)
						rechargeOrderPayEx.m_vecPlatformOrderNumber.push_back(CStringUtil::Utf8ToTStr(pPlatformOrderNumberElement->GetText()));
				}
			}

			TiXmlElement* pPayOrderListElement = pRechargeOrderPayExElement->FirstChildElement("PayOrderList");
			if (pPayOrderListElement != NULL)
			{
				TiXmlElement* pPayOrderElement = NULL;
				for (pPayOrderElement = pPayOrderListElement->FirstChildElement("PayOrder"); pPayOrderElement; pPayOrderElement = pPayOrderElement->NextSiblingElement("PayOrder"))
				{
					CPayOrder payOrder;
					if (pPayOrderElement->Attribute("orderNumber") != NULL)
						payOrder.m_nOrderId = atoi(pPayOrderElement->Attribute("orderNumber"));
					if (pPayOrderElement->Attribute("buyPrice") != NULL)
						payOrder.m_fBuyPrice = atof(pPayOrderElement->Attribute("buyPrice"));

					rechargeOrderPayEx.m_vecPayOrder.push_back(payOrder);
				}
			}

			vecRechargeOrderEx.push_back(rechargeOrderPayEx);
		}

		return true;
	}

	static void BuildGameAccountImportAckBs(int nResult, int nSuccessCount, int nFailCount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement successCountElement("SuccessCount");
		TiXmlElement failCountElement("FailCount");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		InsertTextToEndChild(successCountElement, _T("%d"), nSuccessCount);
		InsertTextToEndChild(failCountElement, _T("%d"), nFailCount);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(successCountElement);
		dataElement.InsertEndChild(failCountElement);

		BuildXmlBs(BuildProtoXmlString(_T("GameAccount_Import_Ack"), dataElement), outByteStream);
	}

	static bool ParseGameAccountImportAckBs(int& nResult, int& nSuccessCount, int& nFailCount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("GameAccount_Import_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pSuccessCountElement = dataElement.FirstChildElement("SuccessCount");
		if (pSuccessCountElement == NULL)
			return false;

		TiXmlElement* pFailCountElement = dataElement.FirstChildElement("FailCount");
		if (pFailCountElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		if (pSuccessCountElement->GetText() != NULL)
			nSuccessCount = atoi(pSuccessCountElement->GetText());

		if (pFailCountElement->GetText() != NULL)
			nFailCount = atoi(pFailCountElement->GetText());

		return true;
	}

	static void BuildGetUserLogAckBs(int nResult, const std::vector<CUserLog>& vecUserLog, int nTotalCount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement userLogListElement("UserLogList");
		for (int i = 0; i < vecUserLog.size(); i++)
		{
			TiXmlElement userLogElement("UserLog");
			userLogElement.SetAttribute("id", vecUserLog[i].m_nId);
			userLogElement.SetAttribute("userId", vecUserLog[i].m_nUserId);
			userLogElement.SetAttribute("type", vecUserLog[i].m_nType);
			userLogElement.SetAttribute("time", CStringUtil::TStrToUtf8(vecUserLog[i].m_strTime).c_str());
			userLogElement.SetAttribute("msg", CStringUtil::TStrToUtf8(vecUserLog[i].m_strMsg).c_str());
			userLogListElement.InsertEndChild(userLogElement);
		}
		TiXmlElement totalCountElement("TotalCount");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		InsertTextToEndChild(totalCountElement, _T("%d"), nTotalCount);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(userLogListElement);
		dataElement.InsertEndChild(totalCountElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_UserLog_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetUserLogAckBs(int& nResult, std::vector<CUserLog>& vecUserLog, int& nTotalCount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_UserLog_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pUserLogListElement = dataElement.FirstChildElement("UserLogList");
		if (pUserLogListElement == NULL)
			return false;

		TiXmlElement* pTotalCountElement = dataElement.FirstChildElement("TotalCount");
		if (pTotalCountElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		if (pTotalCountElement->GetText() != NULL)
			nTotalCount = atoi(pTotalCountElement->GetText());

		TiXmlElement* pUserLogElement = NULL;
		for (pUserLogElement = pUserLogListElement->FirstChildElement("UserLog"); pUserLogElement; pUserLogElement = pUserLogElement->NextSiblingElement("UserLog"))
		{
			CUserLog userLog;
			if (pUserLogElement->Attribute("id") != NULL)
				userLog.m_nId = atoi(pUserLogElement->Attribute("id"));
			if (pUserLogElement->Attribute("userId") != NULL)
				userLog.m_nUserId = atoi(pUserLogElement->Attribute("userId"));
			if (pUserLogElement->Attribute("type") != NULL)
				userLog.m_nType = atoi(pUserLogElement->Attribute("type"));
			if (pUserLogElement->Attribute("time") != NULL)
				userLog.m_strTime = CStringUtil::Utf8ToTStr(pUserLogElement->Attribute("time"));
			if (pUserLogElement->Attribute("msg") != NULL)
				userLog.m_strMsg = CStringUtil::Utf8ToTStr(pUserLogElement->Attribute("msg"));
			vecUserLog.push_back(userLog);
		}

		return true;
	}

	static void BuildGetUserAckBs(int nResult, const std::vector<CUser>& vecUser, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement userListElement("UserList");
		for (int i = 0; i < vecUser.size(); i++)
		{
			TiXmlElement userElement("User");
			userElement.SetAttribute("id", vecUser[i].m_nId);
			userElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecUser[i].m_strUserName).c_str());
			userElement.SetAttribute("roleName", CStringUtil::TStrToUtf8(vecUser[i].m_strRoleName).c_str());
			userElement.SetAttribute("email", CStringUtil::TStrToUtf8(vecUser[i].m_strEmail).c_str());
			userElement.SetAttribute("mobile",  CStringUtil::TStrToUtf8(vecUser[i].m_strMobile).c_str());
			userListElement.InsertEndChild(userElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(userListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_User_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetUserAckBs(int& nResult, std::vector<CUser>& vecUser, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_User_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pUserListElement = dataElement.FirstChildElement("UserList");
		if (pUserListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pUserElement = NULL;
		for (pUserElement = pUserListElement->FirstChildElement("User"); pUserElement; pUserElement = pUserElement->NextSiblingElement("User"))
		{
			CUser user;
			if (pUserElement->Attribute("id") != NULL)
				user.m_nId = atoi(pUserElement->Attribute("id"));
			if (pUserElement->Attribute("userName") != NULL)
				user.m_strUserName = CStringUtil::Utf8ToTStr(pUserElement->Attribute("userName"));
			if (pUserElement->Attribute("roleName") != NULL)
				user.m_strRoleName = CStringUtil::Utf8ToTStr(pUserElement->Attribute("roleName"));
			if (pUserElement->Attribute("email") != NULL)
				user.m_strEmail = CStringUtil::Utf8ToTStr(pUserElement->Attribute("email"));
			if (pUserElement->Attribute("mobile") != NULL)
				user.m_strMobile = CStringUtil::Utf8ToTStr(pUserElement->Attribute("mobile"));
			vecUser.push_back(user);
		}

		return true;
	}

	static void BuildGetUserLogLocationAckBs(int nResult, int nUserId, int nOffset, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement userIdElement("UserId");
		TiXmlElement offsetElement("Offset");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		InsertTextToEndChild(userIdElement, _T("%d"), nUserId);
		InsertTextToEndChild(offsetElement, _T("%d"), nOffset);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(userIdElement);
		dataElement.InsertEndChild(offsetElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_UserLog_Location_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetUserLogLocationAckBs(int& nResult, int& nUserId, int& nOffset, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_UserLog_Location_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pUserIdElement = dataElement.FirstChildElement("UserId");
		if (pUserIdElement == NULL)
			return false;

		TiXmlElement* pOffsetElement = dataElement.FirstChildElement("Offset");
		if (pOffsetElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		if (pUserIdElement->GetText() != NULL)
			nUserId = atoi(pUserIdElement->GetText());

		if (pOffsetElement->GetText() != NULL)
			nOffset = atoi(pOffsetElement->GetText());

		return true;
	}
	
	static void BuildAddUserAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_User_Ack"), dataElement), outByteStream);
	}
	
	static bool ParseAddUserAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_User_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}
	
	static void BuildGetTianHongGameListAckBs(int nResult, const STRMAPGAMEDATA& mapGameList, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement gameListElement("GameList");
		for (STRMAPGAMEDATA::const_iterator mapIter = mapGameList.cbegin(); mapIter != mapGameList.cend(); ++mapIter)
		{
			TiXmlElement gameNameElement("GameName");
			gameNameElement.SetAttribute("name", CStringUtil::TStrToUtf8(mapIter->first).c_str());
			for (std::vector<GAME_DATA>::const_iterator vecIter = mapIter->second.cbegin(); vecIter != mapIter->second.cend(); ++vecIter)
			{
				TiXmlElement gameDataElement("GameData");
				gameDataElement.SetAttribute("bill", vecIter->nGameBill);
				gameDataElement.SetAttribute("type", vecIter->nType);
				for (std::map<int, int>::const_iterator sepMapIter = vecIter->mapSeparate.cbegin(); sepMapIter != vecIter->mapSeparate.cend(); ++sepMapIter)
				{
					TiXmlElement sepElement("GameSepData");
					sepElement.SetAttribute("first", sepMapIter->first);
					sepElement.SetAttribute("second", sepMapIter->second);
					gameDataElement.InsertEndChild(sepElement);
				}
				gameNameElement.InsertEndChild(gameDataElement);
			}

			gameListElement.InsertEndChild(gameNameElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(gameListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_TianHong_GameList_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetTianHongGameListAckBs(int& nResult, STRMAPGAMEDATA& mapGameList, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_TianHong_GameList_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pGameListElement = dataElement.FirstChildElement("GameList");
		if (pGameListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pGameNameElement = NULL;
		for (pGameNameElement = pGameListElement->FirstChildElement("GameName"); pGameNameElement; pGameNameElement = pGameNameElement->NextSiblingElement("GameName"))
		{
			tstring strName;
			if (pGameNameElement->Attribute("name") != NULL)
				strName = CStringUtil::Utf8ToTStr(pGameNameElement->Attribute("name"));

			std::vector<GAME_DATA> vecGameData;
			TiXmlElement* pGameDataElement = NULL;
			for (pGameDataElement = pGameNameElement->FirstChildElement("GameData"); pGameDataElement; pGameDataElement = pGameDataElement->NextSiblingElement("GameData"))
			{
				GAME_DATA gameData;
				if (pGameDataElement->Attribute("bill") != NULL)
					gameData.nGameBill = atoi(pGameDataElement->Attribute("bill"));
				if (pGameDataElement->Attribute("type") != NULL)
					gameData.nType = atoi(pGameDataElement->Attribute("type"));
				TiXmlElement* pGameSepElement = NULL;
				for (pGameSepElement = pGameDataElement->FirstChildElement("GameSepData"); pGameSepElement; pGameSepElement = pGameSepElement->NextSiblingElement("GameSepData"))
				{
					int nFirst, nSecond;
					if (pGameSepElement->Attribute("first") != NULL)
					{
						nFirst = atoi(pGameSepElement->Attribute("first"));
					}
					if (pGameSepElement->Attribute("second") != NULL)
					{
						nSecond = atoi(pGameSepElement->Attribute("second"));
					}
					gameData.mapSeparate.insert(std::pair<int, int>(nFirst, nSecond));
				}
				vecGameData.push_back(gameData);
			}
			
			mapGameList.insert(std::pair<tstring, std::vector<GAME_DATA>>(strName.c_str(), vecGameData));
		}

		return true;
	}

	static void BuildAddTianHongGameInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_TianHong_GameInfo_Ack"), dataElement), outByteStream);
	}

	static bool ParseAddTianHongGameInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_TianHong_GameInfo_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildUpdateTianHongGameInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_TianHong_GameInfo_Ack"), dataElement), outByteStream);
	}

	static bool ParseUpdateTianHongGameInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_TianHong_GameInfo_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildDeleteTianHongGameInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_TianHong_GameInfo_Ack"), dataElement), outByteStream);
	}

	static bool ParseDeleteTianHongGameInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_TianHong_GameInfo_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildGetSmartOrderResultAckBs(int nResult, const std::map<int, int>& mapSepResultList, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		for (std::map<int, int>::const_iterator mapIter = mapSepResultList.cbegin(); mapIter != mapSepResultList.cend(); ++mapIter)
		{
			TiXmlElement sepElement("GameSepData");
			sepElement.SetAttribute("first", mapIter->first);
			sepElement.SetAttribute("second", mapIter->second);

			dataElement.InsertEndChild(sepElement);
		}

		BuildXmlBs(BuildProtoXmlString(_T("Get_SmartOrder_Result_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetSmartOrderResultAckBs(int& nResult, std::map<int, int>& mapSepResultList, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_SmartOrder_Result_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		
		TiXmlElement* pGameSepElement = NULL;
		for (pGameSepElement = dataElement.FirstChildElement("GameSepData"); pGameSepElement; pGameSepElement = pGameSepElement->NextSiblingElement("GameSepData"))
		{
			int nFirst, nSecond;
			if (pGameSepElement->Attribute("first") != NULL)
			{
				nFirst = atoi(pGameSepElement->Attribute("first"));
			}
			if (pGameSepElement->Attribute("second") != NULL)
			{
				nSecond = atoi(pGameSepElement->Attribute("second"));
			}
			mapSepResultList.insert(std::pair<int, int>(nFirst, nSecond));
		}

		return true;
	}

	static void BuildGetReportDataInfoAckBs(int nResult, const std::vector<REPORT_PROFIT>& vecReportInfo, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement reportListElement("ReportList");
		for (std::vector<REPORT_PROFIT>::const_iterator vecIter = vecReportInfo.cbegin(); vecIter != vecReportInfo.cend(); ++vecIter)
		{
			TiXmlElement reportElement("Report");

			reportElement.SetAttribute("date", CStringUtil::TStrToUtf8(vecIter->strData).c_str());

			reportElement.SetAttribute("platform", CStringUtil::TStrToUtf8(vecIter->strPlatform).c_str());
			reportElement.SetDoubleAttribute("platformRatio", vecIter->fPlatformRatio);

			reportElement.SetAttribute("cardName", CStringUtil::TStrToUtf8(vecIter->strCard).c_str());
			reportElement.SetAttribute("cardValue", vecIter->nValue);
			reportElement.SetDoubleAttribute("cardRatio", vecIter->fCardRatio);
			reportElement.SetDoubleAttribute("cardPrice", vecIter->fCardPrice);

			reportElement.SetDoubleAttribute("ticketRatio", vecIter->fTicketRatio);

			reportElement.SetAttribute("channel", CStringUtil::TStrToUtf8(vecIter->strChannel).c_str());
			reportElement.SetDoubleAttribute("channelRatio", vecIter->fChannelRatio);

			reportElement.SetDoubleAttribute("playerDiscount", vecIter->fPlayerDiscount);

			reportListElement.InsertEndChild(reportElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(reportListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_ReportList_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetReportDataInfoAckBs(int& nResult, std::vector<REPORT_PROFIT>& vecReportInfo, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_ReportList_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pReportListElement = dataElement.FirstChildElement("ReportList");
		if (pReportListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pReportElement = NULL;
		for (pReportElement = pReportListElement->FirstChildElement("Report"); pReportElement; pReportElement = pReportElement->NextSiblingElement("Report"))
		{
			REPORT_PROFIT repotProfit;

			if (pReportElement->Attribute("date") != NULL)
				repotProfit.strData = CStringUtil::Utf8ToTStr(pReportElement->Attribute("date"));

			if (pReportElement->Attribute("platform") != NULL)
				repotProfit.strPlatform = CStringUtil::Utf8ToTStr(pReportElement->Attribute("platform"));
			if (pReportElement->Attribute("platformRatio") != NULL)
				repotProfit.fPlatformRatio = atof(pReportElement->Attribute("platformRatio"));

			if (pReportElement->Attribute("cardName") != NULL)
				repotProfit.strCard = CStringUtil::Utf8ToTStr(pReportElement->Attribute("cardName"));
			if (pReportElement->Attribute("cardValue") != NULL)
				repotProfit.nValue = atoi(pReportElement->Attribute("cardValue"));
			if (pReportElement->Attribute("cardRatio") != NULL)
				repotProfit.fCardRatio = atof(pReportElement->Attribute("cardRatio"));
			if (pReportElement->Attribute("cardPrice") != NULL)
				repotProfit.fCardPrice = atof(pReportElement->Attribute("cardPrice"));

			if (pReportElement->Attribute("ticketRatio") != NULL)
				repotProfit.fTicketRatio = atof(pReportElement->Attribute("ticketRatio"));

			if (pReportElement->Attribute("channel") != NULL)
				repotProfit.strChannel = CStringUtil::Utf8ToTStr(pReportElement->Attribute("channel"));
			if (pReportElement->Attribute("channelRatio") != NULL)
				repotProfit.fChannelRatio = atof(pReportElement->Attribute("channelRatio"));

			if (pReportElement->Attribute("playerDiscount") != NULL)
				repotProfit.fPlayerDiscount = atof(pReportElement->Attribute("playerDiscount"));

			vecReportInfo.push_back(repotProfit);			
		}

		return true;
	}

	static void BuildGetReportDataCardListAckBs(int nResult, const CARD_MAP_RATIO& mapCardList, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement cardListElement("CardList");
		for (CARD_MAP_RATIO::const_iterator mapIter = mapCardList.cbegin(); mapIter != mapCardList.cend(); ++mapIter)
		{
			TiXmlElement cardElement("Card");
			cardElement.SetAttribute("name", CStringUtil::TStrToUtf8(mapIter->first).c_str());

			for (std::vector<CARD_RATIO>::const_iterator vecIter = mapIter->second.cbegin(); vecIter != mapIter->second.cend(); ++vecIter)
			{
				TiXmlElement valueElement("Value");
				valueElement.SetAttribute("value", vecIter->nValue);
				valueElement.SetDoubleAttribute("ratio", vecIter->fRatio);
				
				cardElement.InsertEndChild(valueElement);
			}
			cardListElement.InsertEndChild(cardElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(cardListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_CardBusiness_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetReportDataCardListAckBs(int& nResult, CARD_MAP_RATIO& mapCardList, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_CardBusiness_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pCardListElement = dataElement.FirstChildElement("CardList");
		if (pCardListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pCardElement = NULL;
		for (pCardElement = pCardListElement->FirstChildElement("Card"); pCardElement; pCardElement = pCardElement->NextSiblingElement("Card"))
		{
			tstring strName;
			if (pCardElement->Attribute("name") != NULL)
				strName = CStringUtil::Utf8ToTStr(pCardElement->Attribute("name"));

			std::vector<CARD_RATIO> vecValueRatio;

			TiXmlElement* pValueElement = NULL;
			for (pValueElement = pCardElement->FirstChildElement("Value"); pValueElement; pValueElement = pValueElement->NextSiblingElement("Value"))
			{
				CARD_RATIO valueRatio;
				if (pValueElement->Attribute("value") != NULL)
					valueRatio.nValue = atoi(pValueElement->Attribute("value"));
				if (pValueElement->Attribute("ratio") != NULL)
					valueRatio.fRatio = atof(pValueElement->Attribute("ratio"));

				vecValueRatio.push_back(valueRatio);
			}

			mapCardList.insert(std::pair<tstring, std::vector<CARD_RATIO>>(strName.c_str(), vecValueRatio));
		}

		return true;
	}

	static void BuildAddReportDataCardListAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Report_Card_Ack"), dataElement), outByteStream);
	}

	static bool ParseAddReportDataCardListAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Report_Card_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildUpdateReportDataCardNameAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Report_CardName_Ack"), dataElement), outByteStream);
	}

	static bool ParseUpdateReportDataCardNameAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Report_CardName_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildUpdateReportDataCardListAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Report_Card_Ack"), dataElement), outByteStream);
	}

	static bool ParseUpdateReportDataCardListAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Report_Card_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildDeleteReportDataCardListAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Report_Card_Ack"), dataElement), outByteStream);
	}

	static bool ParseDeleteReportDataCardListAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Report_Card_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildGetReportDataChannelListAckBs(int nResult, const STRING_MAP_DOUBLE& mapChannelList, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement channelListElement("ChannelList");
		for (STRING_MAP_DOUBLE::const_iterator mapIter = mapChannelList.cbegin(); mapIter != mapChannelList.cend(); ++mapIter)
		{
			TiXmlElement platformElement("Channel");
			platformElement.SetAttribute("name", CStringUtil::TStrToUtf8(mapIter->first).c_str());
			platformElement.SetDoubleAttribute("ratio", mapIter->second);

			channelListElement.InsertEndChild(platformElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(channelListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_Channel_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetReportDataChannelListAckBs(int& nResult, STRING_MAP_DOUBLE& mapChannelList, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Channel_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pChannelListElement = dataElement.FirstChildElement("ChannelList");
		if (pChannelListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pChannelElement = NULL;
		for (pChannelElement = pChannelListElement->FirstChildElement("Channel"); pChannelElement; pChannelElement = pChannelElement->NextSiblingElement("Channel"))
		{
			tstring strName;
			if (pChannelElement->Attribute("name") != NULL)
				strName = CStringUtil::Utf8ToTStr(pChannelElement->Attribute("name"));

			double ratio;
			if (pChannelElement->Attribute("ratio") != NULL)
			{
				ratio = atof(pChannelElement->Attribute("ratio"));
			}

			mapChannelList.insert(std::pair<tstring, double>(strName.c_str(), ratio));
		}

		return true;
	}

	static void BuildAddReportDataChannelListAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Report_Channel_Ack"), dataElement), outByteStream);
	}

	static bool ParseAddReportDataChannelListAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Report_Channel_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildUpdateReportDataChannelListAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Report_Channel_Ack"), dataElement), outByteStream);
	}

	static bool ParseUpdateReportDataChannelListAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Report_Channel_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildDeleteReportDataChannelListAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Report_Channel_Ack"), dataElement), outByteStream);
	}

	static bool ParseDeleteReportDataChannelListAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Report_Channel_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildGetBackgroundAccountInfoAckBs(int nResult, const std::vector<BACKSTAGE_ACCOUNT_ATTRIBUTE>& vecAccount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement accountListElement("AccountList");
		for (int i = 0; i < vecAccount.size(); i++)
		{
			TiXmlElement accountElement("Account");
			accountElement.SetAttribute("id", vecAccount[i].nId);
			accountElement.SetAttribute("backEm", vecAccount[i].emType);
			accountElement.SetAttribute("type", CStringUtil::TStrToUtf8(vecAccount[i].strType).c_str());
			accountElement.SetAttribute("subType", CStringUtil::TStrToUtf8(vecAccount[i].strSubType).c_str());
			accountElement.SetAttribute("url", CStringUtil::TStrToUtf8(vecAccount[i].strUrl).c_str());
			accountElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecAccount[i].strUserName).c_str());
			accountElement.SetAttribute("password", CStringUtil::TStrToUtf8(vecAccount[i].strPassword).c_str());
			accountElement.SetAttribute("script", CStringUtil::TStrToUtf8(vecAccount[i].strScript).c_str());
			accountListElement.InsertEndChild(accountElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(accountListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_Background_Account_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetBackgroundAccountInfoAckBs(int& nResult, std::vector<BACKSTAGE_ACCOUNT_ATTRIBUTE>& vecAccount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Background_Account_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pAccountListElement = dataElement.FirstChildElement("AccountList");
		if (pAccountListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pAccountElement = NULL;
		for (pAccountElement = pAccountListElement->FirstChildElement("Account"); pAccountElement; pAccountElement = pAccountElement->NextSiblingElement("Account"))
		{
			BACKSTAGE_ACCOUNT_ATTRIBUTE account;
			if (pAccountElement->Attribute("id") != NULL)
				account.nId = atoi(pAccountElement->Attribute("id"));
			if (pAccountElement->Attribute("backEm") != NULL)
				account.emType = (EM_BACKSTAGE_TYPE)atoi(pAccountElement->Attribute("backEm"));
			if (pAccountElement->Attribute("type") != NULL)
				account.strType = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("type"));
			if (pAccountElement->Attribute("subType") != NULL)
				account.strSubType = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("subType"));
			if (pAccountElement->Attribute("url") != NULL)
				account.strUrl = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("url"));
			if (pAccountElement->Attribute("userName") != NULL)
				account.strUserName = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("userName"));
			if (pAccountElement->Attribute("password") != NULL)
			{
				tstring strPassword = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("password"));
				DecryptString(strPassword, account.strPassword);
			}
			if (pAccountElement->Attribute("script") != NULL)
				account.strScript = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("script"));

			vecAccount.push_back(account);
		}

		return true;
	}

	static void BuildAddBackgroundAccountInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Background_Account_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseAddBackgroundAccountInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Background_Account_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildUpdateBackgroundAccountInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Background_Account_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseUpdateBackgroundAccountInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Background_Account_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildDeleteBackgroundAccountInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Background_Account_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseDeleteBackgroundAccountInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Background_Account_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildGetBackStageTypeInfoAckBs(int nResult, const std::vector<BACKSTAGE_ACCOUNT_ATTRIBUTE>& vecAccount, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");
		TiXmlElement accountListElement("AccountList");
		for (int i = 0; i < vecAccount.size(); i++)
		{
			TiXmlElement accountElement("Account");
			accountElement.SetAttribute("id", vecAccount[i].nId);
			accountElement.SetAttribute("backEm", vecAccount[i].emType);
			accountElement.SetAttribute("type", CStringUtil::TStrToUtf8(vecAccount[i].strType).c_str());
			accountElement.SetAttribute("subType", CStringUtil::TStrToUtf8(vecAccount[i].strSubType).c_str());
			accountElement.SetAttribute("url", CStringUtil::TStrToUtf8(vecAccount[i].strUrl).c_str());
			accountElement.SetAttribute("userName", CStringUtil::TStrToUtf8(vecAccount[i].strUserName).c_str());
			accountElement.SetAttribute("password", CStringUtil::TStrToUtf8(vecAccount[i].strPassword).c_str());
			accountElement.SetAttribute("script", CStringUtil::TStrToUtf8(vecAccount[i].strScript).c_str());
			accountListElement.InsertEndChild(accountElement);
		}

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);
		dataElement.InsertEndChild(accountListElement);

		BuildXmlBs(BuildProtoXmlString(_T("Get_Backstage_Type_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseGetBackStageTypeInfoAckBs(int& nResult, std::vector<BACKSTAGE_ACCOUNT_ATTRIBUTE>& vecAccount, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Get_Backstage_Type_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		TiXmlElement* pAccountListElement = dataElement.FirstChildElement("AccountList");
		if (pAccountListElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		TiXmlElement* pAccountElement = NULL;
		for (pAccountElement = pAccountListElement->FirstChildElement("Account"); pAccountElement; pAccountElement = pAccountElement->NextSiblingElement("Account"))
		{
			BACKSTAGE_ACCOUNT_ATTRIBUTE account;
			if (pAccountElement->Attribute("id") != NULL)
				account.nId = atoi(pAccountElement->Attribute("id"));
			if (pAccountElement->Attribute("backEm") != NULL)
				account.emType = (EM_BACKSTAGE_TYPE)atoi(pAccountElement->Attribute("backEm"));
			if (pAccountElement->Attribute("type") != NULL)
				account.strType = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("type"));
			if (pAccountElement->Attribute("subType") != NULL)
				account.strSubType = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("subType"));
			if (pAccountElement->Attribute("url") != NULL)
				account.strUrl = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("url"));
			if (pAccountElement->Attribute("userName") != NULL)
				account.strUserName = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("userName"));
			if (pAccountElement->Attribute("password") != NULL)
				account.strPassword = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("password"));
			if (pAccountElement->Attribute("script") != NULL)
				account.strScript = CStringUtil::Utf8ToTStr(pAccountElement->Attribute("script"));

			vecAccount.push_back(account);
		}

		return true;
	}

	static void BuildAddBackStageTypeInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Add_Backstage_Type_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseAddBackStageTypeInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Add_Backstage_Type_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildUpdateBackStageTypeInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Update_Backstage_Type_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseUpdateBackStageTypeInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Update_Backstage_Type_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}

	static void BuildDeleteBackStageTypeInfoAckBs(int nResult, CByteStream& outByteStream)
	{
		TiXmlElement dataElement("Data");
		TiXmlElement resultElement("Result");

		InsertTextToEndChild(resultElement, _T("%d"), nResult);
		dataElement.InsertEndChild(resultElement);

		BuildXmlBs(BuildProtoXmlString(_T("Delete_Backstage_Type_Info_Ack"), dataElement), outByteStream);
	}

	static bool ParseDeleteBackStageTypeInfoAckBs(int& nResult, CByteStream& inByteStream)
	{
		std::string strXml;
		if (!ParseXmlBs(strXml, inByteStream))
			return false;

		tstring strService;
		TiXmlElement dataElement("");
		if (!CAssistProtocol::ParseProtoXmlString(strXml, strService, dataElement))
			return false;
		if (strService != _T("Delete_Backstage_Type_Info_Ack"))
			return false;

		TiXmlElement* pResultElement = dataElement.FirstChildElement("Result");
		if (pResultElement == NULL)
			return false;

		if (pResultElement->GetText() != NULL)
			nResult = atoi(pResultElement->GetText());

		return true;
	}
};