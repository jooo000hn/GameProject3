﻿#include "stdafx.h"
#include "SkillObject.h"
#include "SceneObject.h"
#include "../StaticData/StaticStruct.h"
#include "../StaticData/StaticData.h"
#include "../Message/Game_Define.pb.h"
#include "../StaticData/StaticStruct.h"
#include "../Scene.h"

CSkillObject::CSkillObject()
{
	m_pCastObject	= NULL;
	m_dwStartTick	= 0;
	m_dwSkillID		= 0;
	m_pSkillInfo	= NULL;
	m_pSkillEventInfo = NULL;
	m_vtTargets.clear();
}

CSkillObject::~CSkillObject()
{
	m_pCastObject	= NULL;
	m_dwStartTick	= 0;
	m_dwSkillID		= 0;
	m_pSkillInfo	= NULL;
	m_pSkillEventInfo = NULL;
	m_vtTargets.clear();
}


BOOL CSkillObject::OnUpdate( UINT64 uTick )
{
	if (m_dwSkillID == 0 || m_pSkillInfo == NULL || m_pSkillEventInfo == NULL)
	{
		return TRUE;
	}

	if (m_dwEventIndex >= m_pSkillEventInfo->vtEvents.size())
	{
		return TRUE;
	}

	UINT64 uElaspedTick = uTick - m_dwStartTick;

	if(uElaspedTick >= m_pSkillEventInfo->vtEvents[m_dwEventIndex].TrigerTime)
	{
		ProcessEvent(m_pSkillEventInfo->vtEvents[m_dwEventIndex]);

		m_dwEventIndex += 1;
	}

	if (uElaspedTick >= m_pSkillInfo->uDuration)
	{
		//响应技能结束
		OnSkillComplete();
	}

	return TRUE;
}

BOOL CSkillObject::OnSkillComplete()
{
	return TRUE;
}

BOOL CSkillObject::StartSkill(UINT32 dwSkillID, INT32 nLevel)
{
	m_pSkillInfo = CStaticData::GetInstancePtr()->GetSkillInfo(dwSkillID, nLevel);
	ERROR_RETURN_FALSE(m_pSkillInfo != NULL);

	m_pSkillEventInfo = CStaticData::GetInstancePtr()->GetSkillEventInfo(dwSkillID);
	ERROR_RETURN_FALSE(m_pSkillEventInfo != NULL);

	m_dwSkillID = dwSkillID;

	m_dwStartTick = CommonFunc::GetTickCount();

	m_dwEventIndex = 0;

	//计算攻击目标
	//1.直接带有目标， 2.需要自己计算目标
	//2.只给自己加buff的技能
	//3.只给目标加buff的技能
	//4.加血的技能
	//5.位移技能
	//6.波次技能(闪电链)
	//7.产生子弹的技能

	OnUpdate(m_dwStartTick);

	return TRUE;
}


BOOL CSkillObject::StopSkill()
{
	m_dwSkillID = 0;
	m_pSkillInfo = 0;
	m_dwEventIndex = 0;
	m_vtTargets.clear();
	return TRUE;
}

BOOL CSkillObject::SetCastObject(CSceneObject* pObject)
{
	m_pCastObject = pObject;

	return TRUE;
}

BOOL CSkillObject::AddTargetObject(CSceneObject* pObject)
{
	m_vtTargets.push_back(pObject);

	return TRUE;
}

BOOL CSkillObject::SkillFight(StSkillEvent& SkillEvent, CSceneObject* pTarget)
{
	ERROR_RETURN_FALSE(m_pCastObject != NULL);
	ERROR_RETURN_FALSE(m_pSkillInfo != NULL);
	ERROR_RETURN_FALSE(pTarget != NULL);

	UINT32 dwRandValue = CommonFunc::GetRandNum(1);
	//先判断是否命中
	if (dwRandValue > (800 + m_pCastObject->m_Propertys[8] - pTarget->m_Propertys[7]) && dwRandValue > 500)
	{
		//未命中
		m_pCastObject->NotifyHitEffect(pTarget, FALSE, 0);
		return TRUE;
	}

	//判断是否爆击
	dwRandValue = CommonFunc::GetRandNum(1);
	BOOL bCriticalHit = FALSE;
	if (dwRandValue < (m_pCastObject->m_Propertys[9] - m_pCastObject->m_Propertys[10]) || dwRandValue < 10)
	{
		bCriticalHit = TRUE;
	}

	//最终伤害加成
	UINT32 dwFinalAdd = m_pCastObject->m_Propertys[6] - pTarget->m_Propertys[5] + 1000;

	//伤害随机
	UINT32 dwFightRand = 900 + CommonFunc::GetRandNum(1) % 200;
	INT32 nHurt = SkillEvent.HurtMuti * m_pCastObject->m_Propertys[5] + SkillEvent.HurtFix;
	nHurt = nHurt - pTarget->m_Propertys[1];
	if (nHurt <= 0)
	{
		nHurt = 1;
	}
	else
	{
		nHurt = nHurt * dwFightRand / 1000;
		nHurt = nHurt * dwFinalAdd / 1000;
		if (bCriticalHit)
		{
			nHurt = nHurt * 15 / 10;
		}
	}

	//pTarget->SubHp(nHurt);
	pTarget->SubHp(nHurt);
	m_pCastObject->NotifyHitEffect(pTarget, bCriticalHit, -nHurt);

	return TRUE;
}

BOOL CSkillObject::CalcTargetObjects(StSkillEvent& SkillEvent)
{
	ERROR_RETURN_FALSE(m_pCastObject != NULL);

	switch (SkillEvent.RangeType)
	{
		case TYPE_OBJECTS:
		{
			//什么都不需要做，直接使用客户端传过来的目标列表
		}
		break;
		case TYPE_CIRCLE:
		{
			FLOAT radius	= SkillEvent.RangeParams[0];
			FLOAT hAngle	= SkillEvent.RangeParams[1];
			FLOAT height	= SkillEvent.RangeParams[2];
			FLOAT offsetX	= SkillEvent.RangeParams[3];
			FLOAT offsetZ	= SkillEvent.RangeParams[4];

			Vector3D hitPoint = m_pCastObject->m_Pos;
			hitPoint = hitPoint + Vector3D(offsetX, 0, offsetZ);

			CScene *pScene = m_pCastObject->GetScene();
			ERROR_RETURN_FALSE(pScene != NULL);

			pScene->SelectTargetsInCircle(m_vtTargets, hitPoint, radius, height);
		}
		break;
		case TYPE_CYLINDER:
		{
			FLOAT radius	= SkillEvent.RangeParams[0];
			FLOAT hAngle	= SkillEvent.RangeParams[1];
			FLOAT height	= SkillEvent.RangeParams[2];
			FLOAT offsetX	= SkillEvent.RangeParams[3];
			FLOAT offsetZ	= SkillEvent.RangeParams[4];

			Vector3D hitPoint = m_pCastObject->m_Pos;
			hitPoint = hitPoint + Vector3D(offsetX, 0, offsetZ);

			FLOAT hitDir = m_pCastObject->m_ft;

			CScene *pScene = m_pCastObject->GetScene();
			ERROR_RETURN_FALSE(pScene != NULL);

			pScene->SelectTargetsInSector(m_vtTargets, hitPoint, hitDir, radius, hAngle);
		}
		break;
		case TYPE_BOX:
		{
			FLOAT length	= SkillEvent.RangeParams[0];
			FLOAT width		= SkillEvent.RangeParams[1];
			FLOAT height	= SkillEvent.RangeParams[2];
			FLOAT offsetX	= SkillEvent.RangeParams[3];
			FLOAT offsetZ	= SkillEvent.RangeParams[4];

			Vector3D hitPoint = m_pCastObject->m_Pos;
			hitPoint = hitPoint + Vector3D(offsetX, 0, offsetZ);

			FLOAT hitDir = m_pCastObject->m_ft;

			CScene *pScene = m_pCastObject->GetScene();
			ERROR_RETURN_FALSE(pScene != NULL);

			pScene->SelectTargetsInSquare(m_vtTargets, hitPoint, hitDir, length, width);
		}
		break;
	}

	return TRUE;
}

BOOL CSkillObject::ProcessEvent(StSkillEvent& SkillEvent)
{
	CalcTargetObjects(SkillEvent);

	if (SkillEvent.SelfBuffID != 0)
	{
		m_pCastObject->AddBuff(SkillEvent.SelfBuffID);
	}

	for (auto itor = m_vtTargets.begin(); itor != m_vtTargets.end(); itor++)
	{
		CSceneObject* pTempObject = *itor;

		if (SkillEvent.TargetBuffID != 0)
		{
			pTempObject->AddBuff(SkillEvent.TargetBuffID);
		}

		SkillFight(SkillEvent, pTempObject);
	}

	for (INT32 nIndex = 0; nIndex < SkillEvent.vtBullets.size(); nIndex++)
	{
		StBulletInfo& data = SkillEvent.vtBullets.at(nIndex);

		CScene* pScene = m_pCastObject->GetScene();

		ERROR_RETURN_FALSE(pScene != NULL);

		pScene->CreateBullet(data.BulletID, m_pCastObject->m_ft + data.Angle, data.BulletType, data.HurtFix, data.HurtMuti);
	}

	return TRUE;
}

