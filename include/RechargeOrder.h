#pragma once

#include "TianHongCardOrder.h"

struct LOG_QUERY_PARAM
{
	std::shared_ptr<int> spNUserId;
	std::shared_ptr<tstring> spStrDuration;
	std::shared_ptr<tstring> spStrMsg;
};

class CUser
{
public:
	CUser()
	{
		this->Reset();
	}

	~CUser()
	{
	}

	void Reset()
	{
		m_nId = 0;
		m_strUserName = _T("");
		m_strRoleName = _T("");
		m_strEmail = _T("");
		m_strMobile = _T("");
	}

public:
	int m_nId;
	tstring m_strUserName;
	tstring m_strRoleName;
	tstring m_strEmail;
	tstring m_strMobile;
};

class CUserLog
{
public:
	CUserLog()
	{
		this->Reset();
	}

	~CUserLog()
	{
	}

	void Reset()
	{
		m_nId = 0;
		m_nUserId = 0;
		m_nType = 0;
		m_strTime = _T("");
		m_strMsg = _T("");
	}

public:
	int m_nId;
	int m_nUserId;
	int m_nType;
	tstring m_strTime;
	tstring m_strMsg;
};

struct ORDER_QUERY_PARAM
{
	std::shared_ptr<tstring> spStrNumber;
	std::shared_ptr<int> spNCustomerId;
	std::shared_ptr<tstring> spStrUserName;
	std::shared_ptr<tstring> spStrState;
	std::shared_ptr<tstring> spStrGameName;
	std::shared_ptr<tstring> spStrChannel;
	std::shared_ptr<tstring> spStrDateStart;
	std::shared_ptr<tstring> spStrDateEnd;
};

class COrderSnapshot
{
public: 
	COrderSnapshot()
	{
		this->Reset();
	}

	~COrderSnapshot()
	{
	}

	void Reset()
	{
		m_nType = 0;
		m_strHashFileName = _T("");
		m_strOriginalFileName = _T("");
		m_strOriginalFilePath = _T("");
	}

public:
	int m_nType;
	tstring m_strHashFileName;
	tstring m_strOriginalFileName;
	tstring m_strOriginalFilePath;
};

class CRechargeOrder
{
public:
	CRechargeOrder()
	{
		this->Reset();
	}

	~CRechargeOrder()
	{
	}

	void Reset()
	{
		m_nId = 0;
		m_nType = 0;
		m_strNumber = _T("");
		m_nCustomerId = 0;
		m_strProductName = _T("");
		m_fPrice = 0;
		m_fPayPrice = 0;
		m_strQQNumber = _T("");
		m_strTelephoneNumber = _T("");
		m_strRoleName = _T("");
		m_strStandbyRoleName = _T("");
		m_strRoleAttri = _T("");

		m_nGameAccountId = 0;
		m_strUserName = _T("");
		m_strPassword = _T("");
		m_strGameName = _T("");
		m_strServiceArea = _T("");
		m_strChannel = _T("");

		m_strState = _T("");
		m_strBuyTime = _T("");
		m_strPayChannel = _T("");
		m_strPayChannelOrderNumber = _T("");
		m_strBankOrderNumber = _T("");
		m_strPayer = _T("");
		m_strDeliveryNote = _T("");
		m_strPayNote = _T("");
		m_strGifeCode = _T("");

		m_fRebateRatio = 0;
		m_vecSnapshot.clear();
		m_vecPlatformOrderNumber.clear();
	}

public:
	int m_nId;							// ID
	int m_nType;						// 订单类型 0:未知类型 1:首充号订单 2:首充号续充订单 3:代充订单
	tstring m_strNumber;				// 订单号
	int m_nCustomerId;					// 购买人ID
	tstring m_strProductName;			// 商品
	double m_fPrice;					// 单价
	double m_fPayPrice;					// 应付金额
	tstring m_strQQNumber;				// QQ
	tstring m_strTelephoneNumber;		// 电话
	tstring m_strRoleName;				// 角色名
	tstring m_strStandbyRoleName;		// 备用角色名
	tstring m_strRoleAttri;				// 附加属性

	int m_nGameAccountId;				// 帐号ID
	tstring m_strUserName;				// 首充账号
	tstring m_strPassword;				// 首充账号密码
	tstring m_strGameName;				// 游戏
	tstring m_strServiceArea;			// 区服
	tstring m_strChannel;				// 渠道
		
	tstring m_strState;					// 订单状态
	tstring m_strBuyTime;				// 购买时间
	tstring m_strPayChannel;			// 支付渠道
	tstring m_strPayChannelOrderNumber;	// 支付渠道订单号
	tstring m_strBankOrderNumber;		// 银行单号
	tstring m_strPayer;					// 支付人
	tstring m_strDeliveryNote;			// 发货备注
	tstring m_strPayNote;				// 支付备注
	tstring m_strGifeCode;				// 赠送礼包码

	double m_fRebateRatio;				// 返利比率
	std::vector<COrderSnapshot> m_vecSnapshot;		// 截图列表
	std::vector<tstring> m_vecPlatformOrderNumber;	// 平台充值订单号列表
};

class CRechargeOrderPayEx : public CRechargeOrder
{
public:
	std::vector<CPayOrder> m_vecPayOrder;
};
