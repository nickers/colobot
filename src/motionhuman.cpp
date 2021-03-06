// * This file is part of the COLOBOT source code
// * Copyright (C) 2001-2008, Daniel ROUX & EPSITEC SA, www.epsitec.ch
// *
// * This program is free software: you can redistribute it and/or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation, either version 3 of the License, or
// * (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program. If not, see  http://www.gnu.org/licenses/.// motionhuman.cpp

#define STRICT
#define D3D_OVERLOADS

#include <windows.h>
#include <stdio.h>
#include <d3d.h>

#include "struct.h"
#include "D3DEngine.h"
#include "math3d.h"
#include "event.h"
#include "misc.h"
#include "iman.h"
#include "light.h"
#include "particule.h"
#include "terrain.h"
#include "water.h"
#include "object.h"
#include "physics.h"
#include "brain.h"
#include "camera.h"
#include "modfile.h"
#include "robotmain.h"
#include "sound.h"
#include "motion.h"
#include "motionhuman.h"



#define ADJUST_ANGLE	FALSE		// TRUE -> ajuste les angles des membres
#define ADJUST_ACTION	(3*3*3*3*MH_SPEC+3*3*3*MHS_SATCOM)

#define START_TIME		1000.0f		// d�but du temps relatif



// Constructeur de l'objet.

CMotionHuman::CMotionHuman(CInstanceManager* iMan, CObject* object)
						  : CMotion(iMan, object)
{
	CMotion::CMotion(iMan, object);

	m_partiReactor   = -1;
	m_armMember      = START_TIME;
	m_armTimeAbs     = START_TIME;
	m_armTimeAction  = START_TIME;
	m_armTimeSwim    = START_TIME;
	m_armTimeIndex   = 0;
	m_armPartIndex   = 0;
	m_armMemberIndex = 0;
	m_armLastAction  = -1;
	m_bArmStop = FALSE;
	m_lastSoundMarch = 0.0f;
	m_lastSoundHhh = 0.0f;
	m_time = 0.0f;
	m_tired = 0.0f;
	m_bDisplayPerso = FALSE;
}

// Destructeur de l'objet.

CMotionHuman::~CMotionHuman()
{
}


// Supprime un objet.

void CMotionHuman::DeleteObject(BOOL bAll)
{
	if ( m_partiReactor != -1 )
	{
		m_particule->DeleteParticule(m_partiReactor);
		m_partiReactor = -1;
	}
}


// D�marre une action.

Error CMotionHuman::SetAction(int action, float time)
{
	CMotion::SetAction(action, time);
	m_time = 0.0f;
	return ERR_OK;
}


// Cr�e le cosmonaute pos� sur le sol.

BOOL CMotionHuman::Create(D3DVECTOR pos, float angle, ObjectType type,
						  float power)
{
	CModFile*	pModFile;
	char		filename[100];
	int			rank, option, face, glasses;

	if ( m_engine->RetRestCreate() < 16 )  return FALSE;

	pModFile = new CModFile(m_iMan);

	m_object->SetType(type);
	option = m_object->RetOption();

	if ( m_main->RetGamerOnlyHead() )
	{
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEVEHICULE);  // c'est un objet mobile
		m_object->SetObjectRank(0, rank);
		face = m_main->RetGamerFace();
		sprintf(filename, "objects\\human2h%d.mod", face+1);
		pModFile->ReadModel(filename);
		pModFile->CreateEngineObject(rank);

		glasses = m_main->RetGamerGlasses();
		if ( glasses != 0 )
		{
			rank = m_engine->CreateObject();
			m_engine->SetObjectType(rank, TYPEDESCENDANT);
			m_object->SetObjectRank(1, rank);
			m_object->SetObjectParent(1, 0);
			sprintf(filename, "objects\\human2g%d.mod", glasses);
			pModFile->ReadModel(filename);
			pModFile->CreateEngineObject(rank);
		}

		CreatePhysics(type);
		m_object->SetFloorHeight(0.0f);

		m_engine->LoadAllTexture();

		delete pModFile;
		return TRUE;
	}

	// Cr�e la base principale.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEVEHICULE);  // c'est un objet mobile
	m_object->SetObjectRank(0, rank);

	if ( option == 0 )  // t�te dans casque ?
	{
		pModFile->ReadModel("objects\\human1c.mod");
	}
	if ( option == 1 )  // t�te � l'air ?
	{
		pModFile->ReadModel("objects\\human1h.mod");
	}
	if ( option == 2 )  // sans sac � dos ?
	{
		pModFile->ReadModel("objects\\human1v.mod");
	}
	pModFile->CreateEngineObject(rank);

	m_object->SetPosition(0, pos);
	m_object->SetAngleY(0, angle);

	// Un v�hicule doit avoir obligatoirement une sph�re de
	// collision avec un centre (0;y;0) (voir GetCrashSphere).
	m_object->CreateCrashSphere(D3DVECTOR(0.0f, 0.0f, 0.0f), 2.0f, SOUND_AIE, 0.20f);
	m_object->SetGlobalSphere(D3DVECTOR(0.0f, 1.0f, 0.0f), 4.0f);

	// Cr�e la t�te.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(1, rank);
	m_object->SetObjectParent(1, 0);

	if ( type == OBJECT_HUMAN )
	{
		if ( option == 0 )  // t�te dans casque ?
		{
			face = m_main->RetGamerFace();
			sprintf(filename, "objects\\human2c%d.mod", face+1);
			pModFile->ReadModel(filename);
		}
		if ( option == 1 ||  // t�te � l'air ?
			 option == 2 )   // sans sac � dos ?
		{
			face = m_main->RetGamerFace();
			sprintf(filename, "objects\\human2h%d.mod", face+1);
			pModFile->ReadModel(filename);
		}
	}
	if ( type == OBJECT_TECH )
	{
		pModFile->ReadModel("objects\\human2t.mod");
	}
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(1, D3DVECTOR(0.0f, 2.7f, 0.0f));
	if ( option == 1 ||  // t�te � l'air ?
		 option == 2 )   // sans sac � dos ?
	{
		m_object->SetZoom(1, D3DVECTOR(1.0f, 1.05f, 1.0f));
	}

	// Cr�e les lunettes.
	glasses = m_main->RetGamerGlasses();
	if ( glasses != 0 && type == OBJECT_HUMAN )
	{
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(15, rank);
		m_object->SetObjectParent(15, 1);
		sprintf(filename, "objects\\human2g%d.mod", glasses);
		pModFile->ReadModel(filename);
		pModFile->CreateEngineObject(rank);
	}

	// Cr�e le bras droite.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(2, rank);
	m_object->SetObjectParent(2, 0);
	pModFile->ReadModel("objects\\human3.mod");
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(2, D3DVECTOR(0.0f, 2.3f, -1.2f));
	m_object->SetAngle(2, D3DVECTOR(90.0f*PI/180.0f, 90.0f*PI/180.0f, -50.0f*PI/180.0f));

	// Cr�e l'avant-bras droite.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(3, rank);
	m_object->SetObjectParent(3, 2);
	pModFile->ReadModel("objects\\human4r.mod");
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(3, D3DVECTOR(1.3f, 0.0f, 0.0f));
	m_object->SetAngle(3, D3DVECTOR(0.0f*PI/180.0f, -20.0f*PI/180.0f, 0.0f*PI/180.0f));

	// Cr�e la main droite.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(4, rank);
	m_object->SetObjectParent(4, 3);
	pModFile->ReadModel("objects\\human5.mod");
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(4, D3DVECTOR(1.2f, 0.0f, 0.0f));

	// Cr�e la cuisse droite.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(5, rank);
	m_object->SetObjectParent(5, 0);
	pModFile->ReadModel("objects\\human6.mod");
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(5, D3DVECTOR(0.0f, 0.0f, -0.7f));
	m_object->SetAngle(5, D3DVECTOR(10.0f*PI/180.0f, 0.0f*PI/180.0f, 5.0f*PI/180.0f));

	// Cr�e la jambe droite.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(6, rank);
	m_object->SetObjectParent(6, 5);
	pModFile->ReadModel("objects\\human7.mod");
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(6, D3DVECTOR(0.0f, -1.5f, 0.0f));
	m_object->SetAngle(6, D3DVECTOR(0.0f*PI/180.0f, 0.0f*PI/180.0f, -10.0f*PI/180.0f));

	// Cr�e le pied droite.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(7, rank);
	m_object->SetObjectParent(7, 6);
	pModFile->ReadModel("objects\\human8.mod");
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(7, D3DVECTOR(0.0f, -1.5f, 0.0f));
	m_object->SetAngle(7, D3DVECTOR(-10.0f*PI/180.0f, 5.0f*PI/180.0f, 5.0f*PI/180.0f));

	// Cr�e le bras gauche.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(8, rank);
	m_object->SetObjectParent(8, 0);
	pModFile->ReadModel("objects\\human3.mod");
	pModFile->Mirror();
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(8, D3DVECTOR(0.0f, 2.3f, 1.2f));
	m_object->SetAngle(8, D3DVECTOR(-90.0f*PI/180.0f, -90.0f*PI/180.0f, -50.0f*PI/180.0f));

	// Cr�e l'avant-bras gauche.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(9, rank);
	m_object->SetObjectParent(9, 8);
	pModFile->ReadModel("objects\\human4l.mod");
	pModFile->Mirror();
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(9, D3DVECTOR(1.3f, 0.0f, 0.0f));
	m_object->SetAngle(9, D3DVECTOR(0.0f*PI/180.0f, 20.0f*PI/180.0f, 0.0f*PI/180.0f));

	// Cr�e la main gauche.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(10, rank);
	m_object->SetObjectParent(10, 9);
	pModFile->ReadModel("objects\\human5.mod");
	pModFile->Mirror();
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(10, D3DVECTOR(1.2f, 0.0f, 0.0f));

	// Cr�e la cuisse gauche.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(11, rank);
	m_object->SetObjectParent(11, 0);
	pModFile->ReadModel("objects\\human6.mod");
	pModFile->Mirror();
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(11, D3DVECTOR(0.0f, 0.0f, 0.7f));
	m_object->SetAngle(11, D3DVECTOR(-10.0f*PI/180.0f, 0.0f*PI/180.0f, 5.0f*PI/180.0f));

	// Cr�e la jambe gauche.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(12, rank);
	m_object->SetObjectParent(12, 11);
	pModFile->ReadModel("objects\\human7.mod");
	pModFile->Mirror();
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(12, D3DVECTOR(0.0f, -1.5f, 0.0f));
	m_object->SetAngle(12, D3DVECTOR(0.0f*PI/180.0f, 0.0f*PI/180.0f, -10.0f*PI/180.0f));

	// Cr�e le pied gauche.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEDESCENDANT);
	m_object->SetObjectRank(13, rank);
	m_object->SetObjectParent(13, 12);
	pModFile->ReadModel("objects\\human8.mod");
	pModFile->Mirror();
	pModFile->CreateEngineObject(rank);
	m_object->SetPosition(13, D3DVECTOR(0.0f, -1.5f, 0.0f));
	m_object->SetAngle(13, D3DVECTOR(10.0f*PI/180.0f, -5.0f*PI/180.0f, 5.0f*PI/180.0f));

	// Cr�e le pistolet.
	if ( option != 2 )  // avec sac � dos ?
	{
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(14, rank);
		m_object->SetObjectParent(14, 0);
		pModFile->ReadModel("objects\\human9.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(14, D3DVECTOR(-1.5f, 0.3f, -1.35f));
		m_object->SetAngleZ(14, PI);
	}

	m_object->CreateShadowCircle(2.0f, 0.8f);

	CreatePhysics(type);
	m_object->SetFloorHeight(0.0f);

	pos = m_object->RetPosition(0);
	m_object->SetPosition(0, pos);  // pour afficher les ombres tout de suite

	m_engine->LoadAllTexture();

	delete pModFile;
	return TRUE;
}

// Cr�e la physique de l'objet.

void CMotionHuman::CreatePhysics(ObjectType type)
{
	Character*	character;
	int			i;

	int member_march[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,	// en l'air :
		90,90,-50,	10,0,55,	0,0,0,		// t0: bras/cuisses/-
		0,-20,0,	-5,0,-110,	0,0,0,		// t0: avant-bras/jambes/-
		0,0,0,		-5,0,40,	0,0,0,		// t0: mains/pieds/-
											// au sol devant :
		125,115,-45,10,0,50,	0,0,0,		// t1: bras/cuisses/-
		0,-20,0,	-5,0,-15,	0,0,0,		// t1: avant-bras/jambes/-
		0,0,0,		-5,0,0,		0,0,0,		// t1: mains/pieds/-
											// au sol derri�re :
		25,55,-40,	10,0,-15,	0,0,0,		// t2: bras/cuisses/-
		30,-50,40,	-5,0,-55,	0,0,0,		// t2: avant-bras/jambes/-
		0,0,0,		-5,0,25,	0,0,0,		// t2: mains/pieds/-
	};

	int member_march_take[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,	// en l'air :
		15,50,-50,	10,0,55,	0,0,0,		// t0: bras/cuisses/-
		45,-70,10,	-5,0,-110,	0,0,0,		// t0: avant-bras/jambes/-
		-10,25,0,	-5,0,40,	0,0,0,		// t0: mains/pieds/-
											// au sol devant :
		15,50,-55,	10,0,50,	0,0,0,		// t1: bras/cuisses/-
		45,-70,10,	-5,0,-15,	0,0,0,		// t1: avant-bras/jambes/-
		-10,25,0,	-5,0,0,		0,0,0,		// t1: mains/pieds/-
											// au sol derri�re :
		15,50,-45,	10,0,-15,	0,0,0,		// t2: bras/cuisses/-
		45,-70,10,	-5,0,-55,	0,0,0,		// t2: avant-bras/jambes/-
		-10,25,0,	-5,0,45,	0,0,0,		// t2: mains/pieds/-
	};

	int member_turn[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,	// en l'air :
		90,90,-50,	10,0,30,	0,0,0,		// t0: bras/cuisses/-
		0,-20,0,	-5,0,-60,	0,0,0,		// t0: avant-bras/jambes/-
		0,0,0,		-5,0,30,	0,0,0,		// t0: mains/pieds/-
											// au sol devant :
		90,110,-45,	10,0,0,		0,0,0,		// t1: bras/cuisses/-
		0,-20,0,	-5,5,0,		0,0,0,		// t1: avant-bras/jambes/-
		0,0,0,		-5,10,0,	0,0,0,		// t1: mains/pieds/-
											// au sol derri�re :
		90,70,-45,	10,0,0,		0,0,0,		// t2: bras/cuisses/-
		0,-20,10,	-5,-5,0,	0,0,0,		// t2: avant-bras/jambes/-
		0,0,0,		-5,-10,0,	0,0,0,		// t2: mains/pieds/-
	};

	int member_stop[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,
		90,90,-50,	10,0,5,		0,0,0,		// bras/cuisses/-
		0,-20,0,	0,0,-10,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// mains/pieds/-
											//
		90,90,-55,	10,0,5,		0,0,0,		// bras/cuisses/-
		0,-15,0,	0,0,-10,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// mains/pieds/-
											//
		90,90,-60,	10,0,5,		0,0,0,		// bras/cuisses/-
		0,-10,0,	0,0,-10,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// mains/pieds/-
	};

	int member_fly[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,
		-5,90,-60,	20,5,-25,	0,0,0,		// bras/cuisses/-
		85,-40,-25,	10,0,-30,	0,0,0,		// avant-bras/jambes/-
		40,10,25,	0,15,0,		0,0,0,		// mains/pieds/-
											//
		-15,90,-40,	20,5,-35,	0,0,0,		// bras/cuisses/-
		85,-40,-25,	10,0,-40,	0,0,0,		// avant-bras/jambes/-
		45,5,20,	0,15,0,		0,0,0,		// mains/pieds/-
											//
		-25,90,-50,	20,5,-20,	0,0,0,		// bras/cuisses/-
		85,-40,-25,	10,0,-10,	0,0,0,		// avant-bras/jambes/-
		30,15,25,	0,15,0,		0,0,0,		// mains/pieds/-
	};

	int member_swim[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,
#if 1
		130,-70,200,10,20,55,	0,0,0,		// bras/cuisses/-
		115,-125,0,	-5,0,-110,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,10,-5,	0,0,0,		// mains/pieds/-
											//
		130,-95,115,55,5,5,		0,0,0,		// bras/cuisses/-
		75,-50,25,	-5,0,-15,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,5,-30,	0,0,0,		// mains/pieds/-
											//
		130,-100,220,5,0,0,		0,0,0,		// bras/cuisses/-
		150,5,0,	-5,0,-15,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,30,-20,	0,0,0,		// mains/pieds/-
#endif
#if 0
		130,-70,200,5,0,0,		0,0,0,		// bras/cuisses/-
		115,-125,0,	-5,0,-15,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,30,-20,	0,0,0,		// mains/pieds/-
											//
		130,-95,115,10,20,55,	0,0,0,		// bras/cuisses/-
		75,-50,25,	-5,0,-110,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,10,-5,	0,0,0,		// mains/pieds/-
											//
		130,-100,220,55,5,5,	0,0,0,		// bras/cuisses/-
		150,5,0,	-5,0,-15,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,5,-30,	0,0,0,		// mains/pieds/-
#endif
#if 0
		130,-70,200,55,5,5,		0,0,0,		// bras/cuisses/-
		115,-125,0,	-5,0,-15,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,5,-30,	0,0,0,		// mains/pieds/-
											//
		130,-95,115,5,0,0,		0,0,0,		// bras/cuisses/-
		75,-50,25,	-5,0,-15,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,30,-20,	0,0,0,		// mains/pieds/-
											//
		130,-100,220,10,20,55,	0,0,0,		// bras/cuisses/-
		150,5,0,	-5,0,-110,	0,0,0,		// avant-bras/jambes/-
		0,0,0,		-5,10,-5,	0,0,0,		// mains/pieds/-
#endif
	};

	int member_spec[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,	// tir :
		65,5,-20,	10,0,40,	0,0,0,		// s0: bras/cuisses/-
		-50,-30,50,	0,0,-70,	0,0,0,		// s0: avant-bras/jambes/-
		0,50,0,		-10,0,35,	0,0,0,		// s0: mains/pieds/-
											// prend arme :
		160,135,-20,10,0,5,		0,0,0,		// s1: bras/cuisses/-
		10,-60,40,	0,0,-10,	0,0,0,		// s1: avant-bras/jambes/-
		0,-5,-25,	-10,5,5,	0,0,0,		// s1: mains/pieds/-
											// porte � terre :
		25,40,-40,	10,0,60,	0,0,0,		// s2: bras/cuisses/-
		0,-45,0,	0,0,-120,	0,0,0,		// s2: avant-bras/jambes/-
		0,15,5,		-10,0,70,	0,0,0,		// s2: mains/pieds/-
											// porte devant :
		25,20,5,	10,0,55,	0,0,0,		// s3: bras/cuisses/-
		-15,-30,10,	0,0,-110,	0,0,0,		// s3: avant-bras/jambes/-
		0,0,0,		-10,0,65,	0,0,0,		// s3: mains/pieds/-
											// porte en hauteur :
		-30,15,-5,	10,0,15,	0,0,0,		// s4: bras/cuisses/-
		0,-15,15,	0,0,-30,	0,0,0,		// s4: avant-bras/jambes/-
		35,0,-15,	-10,0,25,	0,0,0,		// s4: mains/pieds/-
											// se rel�ve :
		15,50,-50,	10,0,5,		0,0,0,		// s5: bras/cuisses/-
		45,-70,10,	0,0,-10,	0,0,0,		// s5: avant-bras/jambes/-
		-10,25,0,	-10,5,5,	0,0,0,		// s5: mains/pieds/-
											// gagn� :
		90,90,-30,	20,0,5,		0,0,0,		// s6: bras/cuisses/-
		0,-90,0,	-10,0,-10,	0,0,0,		// s6: avant-bras/jambes/-
		0,25,0,		-10,5,5,	0,0,0,		// s6: mains/pieds/-
											// perdu :
		-70,45,35,	10,0,40,	0,0,0,		// s7: bras/cuisses/-
		15,-95,-5,	0,0,-70,	0,0,0,		// s7: avant-bras/jambes/-
		0,0,0,		-10,0,35,	0,0,0,		// s7: mains/pieds/-
											// mort par balle (tombe) :
		90,90,-50,	10,0,5,		0,0,0,		// s8: bras/cuisses/-
		0,-20,0,	0,0,-10,	0,0,0,		// s8: avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// s8: mains/pieds/-
											// mort par balle (genoux) :
		110,105,-5,	10,0,25,	0,0,0,		// s9: bras/cuisses/-
		0,-40,20,	0,0,-120,	0,0,0,		// s9: avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// s9: mains/pieds/-
											// mort par balle (genoux) :
		110,120,-25,10,0,25,	0,0,0,		// s10: bras/cuisses/-
		0,-40,20,	0,0,-120,	0,0,0,		// s10: avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// s10: mains/pieds/-
											// mort par balle (plat ventre) :
		110,100,-25,25,0,10,	0,0,0,		// s11: bras/cuisses/-
		0,-40,20,	0,0,-25,	0,0,0,		// s11: avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// s11: mains/pieds/-
											// mort par balle (plat ventre) :
		110,100,-25,25,0,10,	0,0,0,		// s12: bras/cuisses/-
		0,-40,20,	0,0,-25,	0,0,0,		// s12: avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// s12: mains/pieds/-
											// mort noy� :
		110,100,-25,25,0,10,	0,0,0,		// s13: bras/cuisses/-
		0,-40,20,	0,0,-25,	0,0,0,		// s13: avant-bras/jambes/-
		0,0,0,		-10,5,5,	0,0,0,		// s13: mains/pieds/-
											// met/enl�ve drapeau :
		85,45,-50,	10,0,60,	0,0,0,		// s14: bras/cuisses/-
		-60,15,65,	0,0,-105,	0,0,0,		// s14: avant-bras/jambes/-
		0,10,0,		-10,0,60,	0,0,0,		// s14: mains/pieds/-
											// lit SatCom :
		70,30,-20,	10,0,5,		0,0,0,		// s15: bras/cuisses/-
		115,-65,60,	0,0,-10,	0,0,0,		// s15: avant-bras/jambes/-
		0,20,0,		-10,5,5,	0,0,0,		// s15: mains/pieds/-
	};

	m_physics->SetType(TYPE_FLYING);

	character = m_object->RetCharacter();
	character->wheelFront = 4.0f;
	character->wheelBack  = 4.0f;
	character->wheelLeft  = 4.0f;
	character->wheelRight = 4.0f;
	character->height     = 3.5f;

	if ( type == OBJECT_HUMAN )
	{
		m_physics->SetLinMotionX(MO_ADVSPEED, 50.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 35.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 70.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 40.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 40.0f);
		m_physics->SetLinMotionY(MO_ADVSPEED, 60.0f);
		m_physics->SetLinMotionY(MO_RECSPEED, 60.0f);
		m_physics->SetLinMotionY(MO_ADVACCEL, 20.0f);
		m_physics->SetLinMotionY(MO_RECACCEL, 50.0f);
		m_physics->SetLinMotionY(MO_STOACCEL, 50.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.8f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.8f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  6.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  6.0f);
		m_physics->SetCirMotionY(MO_STOACCEL,  4.0f);
	}
	else
	{
		m_physics->SetLinMotionX(MO_ADVSPEED, 40.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 15.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL,  8.0f);
		m_physics->SetLinMotionX(MO_RECACCEL,  8.0f);
		m_physics->SetLinMotionX(MO_STOACCEL,  8.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 50.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 50.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 40.0f);
		m_physics->SetLinMotionY(MO_ADVSPEED, 60.0f);
		m_physics->SetLinMotionY(MO_RECSPEED, 60.0f);
		m_physics->SetLinMotionY(MO_ADVACCEL, 20.0f);
		m_physics->SetLinMotionY(MO_RECACCEL, 50.0f);
		m_physics->SetLinMotionY(MO_STOACCEL, 50.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.6f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.6f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  4.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  4.0f);
		m_physics->SetCirMotionY(MO_STOACCEL,  3.0f);
	}

	for ( i=0 ; i<3*3*3*3 ; i++ )
	{
		m_armAngles[3*3*3*3*MH_MARCH+i] = member_march[i];
	}
	for ( i=0 ; i<3*3*3*3 ; i++ )
	{
		m_armAngles[3*3*3*3*MH_MARCHTAKE+i] = member_march_take[i];
	}
	for ( i=0 ; i<3*3*3*3 ; i++ )
	{
		m_armAngles[3*3*3*3*MH_TURN+i] = member_turn[i];
	}
	for ( i=0 ; i<3*3*3*3 ; i++ )
	{
		m_armAngles[3*3*3*3*MH_STOP+i] = member_stop[i];
	}
	for ( i=0 ; i<3*3*3*3 ; i++ )
	{
		m_armAngles[3*3*3*3*MH_FLY+i] = member_fly[i];
	}
	for ( i=0 ; i<3*3*3*3 ; i++ )
	{
		m_armAngles[3*3*3*3*MH_SWIM+i] = member_swim[i];
	}
	for ( i=0 ; i<3*3*3*16 ; i++ )
	{
		m_armAngles[3*3*3*3*MH_SPEC+i] = member_spec[i];
	}
}


// Gestion d'un �v�nement.

BOOL CMotionHuman::EventProcess(const Event &event)
{
	CMotion::EventProcess(event);

	if ( event.event == EVENT_FRAME )
	{
		return EventFrame(event);
	}

	if ( event.event == EVENT_KEYDOWN )
	{
#if ADJUST_ANGLE
		int		i;

		if ( event.param == 'A' )  m_armTimeIndex++;
		if ( m_armTimeIndex >= 3 )  m_armTimeIndex = 0;

		if ( event.param == 'Q' )  m_armPartIndex++;
		if ( m_armPartIndex >= 3 )  m_armPartIndex = 0;

		if ( event.param == 'W' )  m_armMemberIndex++;
		if ( m_armMemberIndex >= 3 )  m_armMemberIndex = 0;

		i  = m_armMemberIndex*3;
		i += m_armPartIndex*3*3;
		i += m_armTimeIndex*3*3*3;
		i += ADJUST_ACTION;

		if ( event.param == 'E' )  m_armAngles[i+0] += 5;
		if ( event.param == 'D' )  m_armAngles[i+0] -= 5;
		if ( event.param == 'R' )  m_armAngles[i+1] += 5;
		if ( event.param == 'F' )  m_armAngles[i+1] -= 5;
		if ( event.param == 'T' )  m_armAngles[i+2] += 5;
		if ( event.param == 'G' )  m_armAngles[i+2] -= 5;

		if ( event.param == 'Y' )  m_bArmStop = !m_bArmStop;

		if ( event.param == 'Y' )
		{
			char s[100];
			sprintf(s, "index dans table = %d %d %d\n", i, i+9, i+18);
			OutputDebugString(s);
		}
#endif
	}

	return TRUE;
}

// Calcule une valeur (radians) proportionnelle comprise
// entre a et b (degr�s).

inline float Propf(float a, float b, float p)
{
	float	aa, bb;

	aa = a*PI/180.0f;
	bb = b*PI/180.0f;

	return aa+p*(bb-aa);
}

// Gestion d'un �v�nement.

BOOL CMotionHuman::EventFrame(const Event &event)
{
	D3DMATRIX*	mat;
	D3DVECTOR	dir, actual, pos, speed, pf;
	FPOINT		center, dim, p2;
	float		s, a, prog, rTime[2], lTime[2], time, rot, hr, hl;
	float		al, ar, af;
	float		tSt[9], tNd[9];
	float		aa, bb, shield, deadFactor, level;
	int			i, ii, st, nd, action, legAction, armAction;
	BOOL		bOnBoard, bSwim, bStop;

	if ( m_engine->RetPause() )
	{
		if ( m_actionType == MHS_SATCOM )
		{
			m_progress += event.rTime*m_actionTime;
		}
		else
		{
			return TRUE;
		}
	}

	bOnBoard = FALSE;
	if ( m_object->RetSelect() &&
		 m_camera->RetType() == CAMERA_ONBOARD )
	{
		bOnBoard = TRUE;
	}

	if ( m_bDisplayPerso && m_main->RetGamerOnlyHead() )
	{
		m_time += event.rTime;
		m_object->SetLinVibration(D3DVECTOR(0.0f, -0.55f, 0.0f));
		m_object->SetCirVibration(D3DVECTOR(0.0f, m_main->RetPersoAngle(), 0.0f));
		return TRUE;
	}
	if ( m_bDisplayPerso )
	{
		m_object->SetCirVibration(D3DVECTOR(0.0f, m_main->RetPersoAngle()+0.2f, 0.0f));
	}

	shield = m_object->RetShield();
	shield += event.rTime*(1.0f/120.0f);  // r�g�n�ration en 120 secondes
	if ( shield > 1.0f )  shield = 1.0f;
	m_object->SetShield(shield);

	bSwim = m_physics->RetSwim();

#if 0
	rot = m_physics->RetCirMotionY(MO_MOTSPEED);
	s = m_physics->RetLinMotionX(MO_REASPEED)*2.0f;
	a = m_physics->RetLinMotionX(MO_TERSPEED);
	if ( a < 0.0f )  // monte ?
	{
		if ( s > 0.0f && s <  20.0f )  s = 20.0f;  // avance lentement ?
//?		if ( s < 0.0f && s > -10.0f )  s =  0.0f;  // recule lentement ?
	}
	if ( a > 0.0f && !bSwim )  // descend ?
	{
		if ( s > 0.0f && s <  10.0f )  s =   0.0f;  // avance lentement ?
//?		if ( s < 0.0f && s >  -5.0f )  s =  -5.0f;  // recule lentement ?
	}
	a = Abs(rot*12.0f);

	if ( !m_physics->RetLand() && !bSwim )  // en vol ?
	{
		s = 0.0f;
	}

	if ( m_object->RetFret() != 0 )  // porte qq chose ?
	{
		s *= 1.3f;
	}
#else
	rot = m_physics->RetCirMotionY(MO_MOTSPEED);
#if 0
	s = m_physics->RetLinMotionX(MO_REASPEED);
#else
	a = m_physics->RetLinMotionX(MO_REASPEED);
	s = m_physics->RetLinMotionX(MO_MOTSPEED)*0.2f;
	if ( Abs(a) > Abs(s) )  s = a;  // la plus grande valeur
#endif
	a = m_physics->RetLinMotionX(MO_TERSPEED);
	if ( a < 0.0f )  // monte ?
	{
		a += m_physics->RetLinMotionX(MO_TERSLIDE);
		if ( a < 0.0f )  s -= a;
	}
	if ( a > 0.0f )  // descend ?
	{
		a -= m_physics->RetLinMotionX(MO_TERSLIDE);
		if ( a > 0.0f )  s -= a;
	}
	s *= 2.0f;
	a = Abs(rot*12.0f);

	if ( !m_physics->RetLand() && !bSwim )  // en vol ?
	{
		s = 0.0f;
	}

	if ( m_object->RetFret() != 0 )  // porte qq chose ?
	{
		s *= 1.3f;
	}
#endif

	m_time += event.rTime;
	m_armTimeAbs += event.rTime;
	m_armTimeAction += event.rTime;
	m_armMember += s*event.rTime*0.05f;

	// Gestion de la fatigue lorsqu'on court.
	if ( m_physics->RetLand() && s != 0.0f )  // au sol ?
	{
		m_tired += event.rTime*0.1f;
		if ( m_tired > 1.0f )
		{
			m_tired = 1.0f;
			if ( m_lastSoundHhh > 3.0f )  m_lastSoundHhh = 0.5f;
		}
	}
	else
	{
		m_tired -= event.rTime*0.2f;
		if ( m_tired < 0.0f )  m_tired = 0.0f;
	}

	if ( bSwim )  // nage ?
	{
		s += Abs(m_physics->RetLinMotionY(MO_REASPEED)*2.0f);
		a *= 2.0f;
		m_armTimeSwim += Min(Max(s,a,3.0f),15.0f)*event.rTime*0.05f;
	}

	bStop = ( s == 0.0f );  // � l'arr�t ?
	prog = 0.0f;

	if ( m_physics->RetLand() )  // au sol ?
	{
		if ( s == 0.0f && a == 0.0f )
		{
			action = MH_STOP;  // stop
			rTime[0] = rTime[1] = m_armTimeAbs*0.21f;
			lTime[0] = lTime[1] = m_armTimeAbs*0.25f;
			m_armMember = START_TIME;
		}
		else
		{
			if ( s == 0.0f )
			{
				action = MH_TURN;  // turn
				rTime[0] = rTime[1] = m_armTimeAbs;
				lTime[0] = lTime[1] = m_armTimeAbs+0.5f;
				if ( rot < 0.0f )
				{
					rTime[1] = 1000000.0f-rTime[1];
				}
				else
				{
					lTime[1] = 1000000.0f-lTime[1];
				}
				m_armMember = START_TIME;
			}
			else
			{
				action = MH_MARCH;  // march
				if ( m_object->RetFret() != 0 )  action = MH_MARCHTAKE;  // march-take
				rTime[0] = rTime[1] = m_armMember;
				lTime[0] = lTime[1] = m_armMember+0.5f;
			}
		}
		if ( bSwim )
		{
			rTime[0] *= 0.6f;
			rTime[1] *= 0.6f;
			lTime[0] = rTime[0]+0.5f;
			lTime[1] = rTime[1]+0.5f;
		}
	}
	else
	{
		if ( bSwim )
		{
			action = MH_SWIM;  // nage
			rTime[0] = rTime[1] = m_armTimeSwim;
			lTime[0] = lTime[1] = m_armTimeSwim;
		}
		else
		{
			action = MH_FLY;  // fly
			rTime[0] = rTime[1] = m_armTimeAbs*0.30f;
			lTime[0] = lTime[1] = m_armTimeAbs*0.31f;
			m_armMember = START_TIME;
		}
	}

	if ( action != m_armLastAction )
	{
		m_armLastAction = action;
		m_armTimeAction = 0.0f;
	}

	armAction = action;
	legAction = action;

	if ( m_object->RetFret() != 0 )  // porte qq chose ?
	{
		armAction = MH_MARCHTAKE;  // march-take
	}
	
	if ( m_physics->RetLand() )  // au sol ?
	{
		a = m_object->RetAngleY(0);
		pos = m_object->RetPosition(0);
		m_terrain->MoveOnFloor(pos);

		pf.x = pos.x+cosf(a+PI*1.5f)*0.7f;
		pf.y = pos.y;
		pf.z = pos.z-sinf(a+PI*1.5f)*0.7f;
		m_terrain->MoveOnFloor(pf);
		al = atanf((pf.y-pos.y)/0.7f);  // angle pour jambe gauche

		pf = pos;
		pf.x = pos.x+cosf(a+PI*0.5f)*0.7f;
		pf.y = pos.y;
		pf.z = pos.z-sinf(a+PI*0.5f)*0.7f;
		m_terrain->MoveOnFloor(pf);
		ar = atanf((pf.y-pos.y)/0.7f);  // angle pour jambe droite

		pf.x = pos.x+cosf(a+PI)*0.3f;
		pf.y = pos.y;
		pf.z = pos.z-sinf(a+PI)*0.3f;
		m_terrain->MoveOnFloor(pf);
		af = atanf((pf.y-pos.y)/0.3f);  // angle pour pieds
	}
	else
	{
		al = 0.0f;
		ar = 0.0f;
		af = 0.0f;
	}
	
	for ( i=0 ; i<4 ; i++ )  // les 4 membres
	{
		if ( m_bArmStop )  // mise au point ?
		{
			st = ADJUST_ACTION + (i%2)*3;
			nd = st;
			time = 100.0f;
			m_armTimeAction = 0.0f;
		}
		else if ( m_actionType != -1 )  // action sp�ciale en cours ?
		{
			st = 3*3*3*3*MH_SPEC + 3*3*3*m_actionType + (i%2)*3;
			nd = st;
			time = event.rTime*m_actionTime;
			m_armTimeAction = 0.0f;
		}
		else
		{
			if ( i < 2 )  prog = Mod(rTime[i%2], 1.0f);
			else          prog = Mod(lTime[i%2], 1.0f);
			if ( prog < 0.25f )  // t0..t1 ?
			{
				prog = prog/0.25f;  // 0..1
				st = 0;  // index start
				nd = 1;  // index end
			}
			else if ( prog < 0.75f )  // t1..t2 ?
			{
				prog = (prog-0.25f)/0.50f;  // 0..1
				st = 1;  // index start
				nd = 2;  // index end
			}
			else	// t2..t0 ?
			{
				prog = (prog-0.75f)/0.25f;  // 0..1
				st = 2;  // index start
				nd = 0;  // index end
			}
			if ( i%2 == 0 )  // bras ?
			{
				st = 3*3*3*3*armAction + st*3*3*3 + (i%2)*3;
				nd = 3*3*3*3*armAction + nd*3*3*3 + (i%2)*3;
			}
			else	// jambe ?
			{
				st = 3*3*3*3*legAction + st*3*3*3 + (i%2)*3;
				nd = 3*3*3*3*legAction + nd*3*3*3 + (i%2)*3;
			}

			// De moins en moins mou ...
			time = event.rTime*(5.0f+Min(m_armTimeAction*50.0f, 100.0f));
			if ( bSwim )  time *= 0.25f;
		}

		tSt[0] = m_armAngles[st+ 0];  // x
		tSt[1] = m_armAngles[st+ 1];  // y
		tSt[2] = m_armAngles[st+ 2];  // z
		tSt[3] = m_armAngles[st+ 9];  // x
		tSt[4] = m_armAngles[st+10];  // y
		tSt[5] = m_armAngles[st+11];  // z
		tSt[6] = m_armAngles[st+18];  // x
		tSt[7] = m_armAngles[st+19];  // y
		tSt[8] = m_armAngles[st+20];  // z

		tNd[0] = m_armAngles[nd+ 0];  // x
		tNd[1] = m_armAngles[nd+ 1];  // y
		tNd[2] = m_armAngles[nd+ 2];  // z
		tNd[3] = m_armAngles[nd+ 9];  // x
		tNd[4] = m_armAngles[nd+10];  // y
		tNd[5] = m_armAngles[nd+11];  // z
		tNd[6] = m_armAngles[nd+18];  // x
		tNd[7] = m_armAngles[nd+19];  // y
		tNd[8] = m_armAngles[nd+20];  // z

		aa = 0.5f;
		if ( i%2 == 0 )  // bras ?
		{
			if ( m_object->RetFret() == 0 )   // ne porte rien ?
			{
				aa = 2.0f;  // bouge beaucoup
			}
			else
			{
				aa = 0.0f;  // immobile
			}
		}

		if ( i < 2 )  // gauche ?
		{
			bb = sinf(m_time*1.1f)*aa;  tSt[0] += bb;  tNd[0] += bb;
			bb = sinf(m_time*1.0f)*aa;  tSt[1] += bb;  tNd[1] += bb;
			bb = sinf(m_time*1.2f)*aa;  tSt[2] += bb;  tNd[2] += bb;
			bb = sinf(m_time*2.5f)*aa;  tSt[3] += bb;  tNd[3] += bb;
			bb = sinf(m_time*2.0f)*aa;  tSt[4] += bb;  tNd[4] += bb;
			bb = sinf(m_time*3.8f)*aa;  tSt[5] += bb;  tNd[5] += bb;
			bb = sinf(m_time*3.0f)*aa;  tSt[6] += bb;  tNd[6] += bb;
			bb = sinf(m_time*2.3f)*aa;  tSt[7] += bb;  tNd[7] += bb;
			bb = sinf(m_time*4.0f)*aa;  tSt[8] += bb;  tNd[8] += bb;
		}
		else	// droite ?
		{
			bb = sinf(m_time*0.9f)*aa;  tSt[0] += bb;  tNd[0] += bb;
			bb = sinf(m_time*1.2f)*aa;  tSt[1] += bb;  tNd[1] += bb;
			bb = sinf(m_time*1.4f)*aa;  tSt[2] += bb;  tNd[2] += bb;
			bb = sinf(m_time*2.9f)*aa;  tSt[3] += bb;  tNd[3] += bb;
			bb = sinf(m_time*1.4f)*aa;  tSt[4] += bb;  tNd[4] += bb;
			bb = sinf(m_time*3.1f)*aa;  tSt[5] += bb;  tNd[5] += bb;
			bb = sinf(m_time*3.7f)*aa;  tSt[6] += bb;  tNd[6] += bb;
			bb = sinf(m_time*2.0f)*aa;  tSt[7] += bb;  tNd[7] += bb;
			bb = sinf(m_time*3.1f)*aa;  tSt[8] += bb;  tNd[8] += bb;
		}

#if 1
		if ( i%2 == 1           &&  // jambe ?
			 m_actionType == -1 )   // pas action sp�ciale ?
		{
			if ( i == 1 )  // jambe droite ?
			{
				ii = 5;
				a = ar*0.25f;
			}
			else
			{
				ii = 11;
				a = al*0.25f;
			}
			if ( a < -0.2f )  a = -0.2f;
			if ( a >  0.2f )  a =  0.2f;

			pos = m_object->RetPosition(ii+0);
			pos.y = 0.0f+a;
			m_object->SetPosition(ii+0, pos);  // allonge/raccourci cuisse

			pos = m_object->RetPosition(ii+1);
			pos.y = -1.5f+a;
			m_object->SetPosition(ii+1, pos);  // allonge/raccourci jambe

			pos = m_object->RetPosition(ii+2);
			pos.y = -1.5f+a;
			m_object->SetPosition(ii+2, pos);  // allonge/raccourci pied

			if ( i == 1 )  // jambe droite ?
			{
				aa = (ar*180.0f/PI*0.5f);
			}
			else	// jambe gauche ?
			{
				aa = (al*180.0f/PI*0.5f);
			}
			tSt[6] += aa;
			tNd[6] += aa;  // augmente l'angle X du pied

			if ( i == 1 )  // jambe droite ?
			{
				aa = (ar*180.0f/PI);
			}
			else	// jambe gauche ?
			{
				aa = (al*180.0f/PI);
			}
			if ( aa <  0.0f )  aa =  0.0f;
			if ( aa > 30.0f )  aa = 30.0f;

			tSt[2] += aa;
			tNd[2] += aa;    // augmente l'angle Z de la cuisse
			tSt[5] -= aa*2;
			tNd[5] -= aa*2;  // augmente l'angle Z de la jambe
			tSt[8] += aa;
			tNd[8] += aa;    // augmente l'angle Z du pied

			aa = (af*180.0f/PI)*0.7f;
			if ( aa < -30.0f )  aa = -30.0f;
			if ( aa >  30.0f )  aa =  30.0f;

			tSt[8] -= aa;
			tNd[8] -= aa;    // augmente l'angle Z du pied
		}
#endif

		if ( m_actionType == MHS_DEADw )   // mort noy� ?
		{
			if ( m_progress < 0.5f )
			{
				deadFactor = m_progress/0.5f;
			}
			else
			{
				deadFactor = 1.0f-(m_progress-0.5f)/0.5f;
			}
			if ( deadFactor < 0.0f )  deadFactor = 0.0f;
			if ( deadFactor > 1.0f )  deadFactor = 1.0f;

			for ( ii=0 ; ii<9 ; ii++ )
			{
				tSt[ii] += Rand()*20.0f*deadFactor;
				tNd[ii] = tSt[ii];
			}
			time = 100.0f;
		}

		if ( i < 2 )  // membre droite (0..1) ?
		{
			m_object->SetAngleX(2+3*i+0, Smooth(m_object->RetAngleX(2+3*i+0), Propf(tSt[0], tNd[0], prog), time));
			m_object->SetAngleY(2+3*i+0, Smooth(m_object->RetAngleY(2+3*i+0), Propf(tSt[1], tNd[1], prog), time));
			m_object->SetAngleZ(2+3*i+0, Smooth(m_object->RetAngleZ(2+3*i+0), Propf(tSt[2], tNd[2], prog), time));
			m_object->SetAngleX(2+3*i+1, Smooth(m_object->RetAngleX(2+3*i+1), Propf(tSt[3], tNd[3], prog), time));
			m_object->SetAngleY(2+3*i+1, Smooth(m_object->RetAngleY(2+3*i+1), Propf(tSt[4], tNd[4], prog), time));
			m_object->SetAngleZ(2+3*i+1, Smooth(m_object->RetAngleZ(2+3*i+1), Propf(tSt[5], tNd[5], prog), time));
			m_object->SetAngleX(2+3*i+2, Smooth(m_object->RetAngleX(2+3*i+2), Propf(tSt[6], tNd[6], prog), time));
			m_object->SetAngleY(2+3*i+2, Smooth(m_object->RetAngleY(2+3*i+2), Propf(tSt[7], tNd[7], prog), time));
			m_object->SetAngleZ(2+3*i+2, Smooth(m_object->RetAngleZ(2+3*i+2), Propf(tSt[8], tNd[8], prog), time));
		}
		else	// membre gauche (2..3) ?
		{
			m_object->SetAngleX(2+3*i+0, Smooth(m_object->RetAngleX(2+3*i+0), Propf(-tSt[0], -tNd[0], prog), time));
			m_object->SetAngleY(2+3*i+0, Smooth(m_object->RetAngleY(2+3*i+0), Propf(-tSt[1], -tNd[1], prog), time));
			m_object->SetAngleZ(2+3*i+0, Smooth(m_object->RetAngleZ(2+3*i+0), Propf( tSt[2],  tNd[2], prog), time));
			m_object->SetAngleX(2+3*i+1, Smooth(m_object->RetAngleX(2+3*i+1), Propf(-tSt[3], -tNd[3], prog), time));
			m_object->SetAngleY(2+3*i+1, Smooth(m_object->RetAngleY(2+3*i+1), Propf(-tSt[4], -tNd[4], prog), time));
			m_object->SetAngleZ(2+3*i+1, Smooth(m_object->RetAngleZ(2+3*i+1), Propf( tSt[5],  tNd[5], prog), time));
			m_object->SetAngleX(2+3*i+2, Smooth(m_object->RetAngleX(2+3*i+2), Propf(-tSt[6], -tNd[6], prog), time));
			m_object->SetAngleY(2+3*i+2, Smooth(m_object->RetAngleY(2+3*i+2), Propf(-tSt[7], -tNd[7], prog), time));
			m_object->SetAngleZ(2+3*i+2, Smooth(m_object->RetAngleZ(2+3*i+2), Propf( tSt[8],  tNd[8], prog), time));
		}
	}

#if ADJUST_ANGLE
	if ( m_object->RetSelect() )
	{
		char s[100];
		sprintf(s, "A:time=%d Q:part=%d W:member=%d", m_armTimeIndex, m_armPartIndex, m_armMemberIndex);
		m_engine->SetInfoText(4, s);
	}
#endif

	// Calcule la hauteur de rabaissement en fonction de la
	// position des jambes.
	hr = 1.5f*(1.0f-cosf(m_object->RetAngleZ(5))) +
		 1.5f*(1.0f-cosf(m_object->RetAngleZ(5)+m_object->RetAngleZ(6)));
	a = 1.0f*sinf(m_object->RetAngleZ(5)+m_object->RetAngleZ(6)+m_object->RetAngleZ(7));
	if ( a < 0.0f )  hr += a;

	hl = 1.5f*(1.0f-cosf(m_object->RetAngleZ(11))) +
		 1.5f*(1.0f-cosf(m_object->RetAngleZ(11)+m_object->RetAngleZ(12)));
	a = 1.0f*sinf(m_object->RetAngleZ(11)+m_object->RetAngleZ(12)+m_object->RetAngleZ(13));
	if ( a < 0.0f )  hl += a;

	hr = Min(hr, hl);

	if ( m_actionType == MHS_FIRE )  // tir ?
	{
		time = event.rTime*m_actionTime;

		dir.x = (Rand()-0.5f)/8.0f;
		dir.z = (Rand()-0.5f)/8.0f;
		dir.y = -0.5f;  // l�g�rement plus bas
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
//?		dir.y = Smooth(actual.y, dir.y, time);
		dir.y = -hr;
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.x = 0.0f;
		dir.y = (Rand()-0.5f)/3.0f;
		dir.z = -0.1f;  // l�g�rement pench� en avant
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_TAKE      ||  // prend ?
			  m_actionType == MHS_TAKEOTHER )   // drapeau ?
	{
		time = event.rTime*m_actionTime*2.0f;

		dir.x = 0.0f;
		dir.z = 0.0f;
		dir.y = -1.5f;  // l�g�rement plus bas
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
//?		dir.y = Smooth(actual.y, dir.y, time);
		dir.y = -hr;
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.x = 0.0f;
		dir.y = 0.0f;
		dir.z = -0.2f;
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_TAKEHIGH )   // prend ?
	{
		time = event.rTime*m_actionTime*2.0f;

		dir.x = 0.4f;  // avance l�g�rement
		dir.z = 0.0f;
		dir.y = 0.0f;
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
//?		dir.y = Smooth(actual.y, dir.y, time);
		dir.y = -hr;
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.x = 0.0f;
		dir.y = 0.0f;
		dir.z = -0.2f;
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_FLAG )   // drapeau ?
	{
		time = event.rTime*m_actionTime*2.0f;

		dir.x = 0.0f;
		dir.z = 0.0f;
		dir.y = -2.0f;  // l�g�rement plus bas
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
//?		dir.y = Smooth(actual.y, dir.y, time);
		dir.y = -hr;
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.x = 0.0f;
		dir.y = 0.0f;
		dir.z = -0.4f;
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_DEADg )   // mort par balle (tombe) ?
	{
		if ( m_physics->RetLand() )  // au sol ?
		{
			SetAction(MHS_DEADg1, 0.5f);  // genoux
		}
	}
	else if ( m_actionType == MHS_DEADg1 )   // mort par balle (genoux) ?
	{
		prog = m_progress;
		if ( prog >= 1.0f )
		{
			prog = 1.0f;

			for ( i=0 ; i<10 ; i++ )
			{
				pos = m_object->RetPosition(0);
				pos.x += (Rand()-0.5f)*4.0f;
				pos.z += (Rand()-0.5f)*4.0f;
				m_terrain->MoveOnFloor(pos);
				speed = D3DVECTOR(0.0f, 0.0f, 0.0f);
				dim.x = 1.2f+Rand()*1.2f;
				dim.y = dim.x;
				m_particule->CreateParticule(pos, speed, dim, PARTICRASH, 2.0f, 0.0f, 0.0f);
			}
			m_sound->Play(SOUND_BOUMv, m_object->RetPosition(0));

			SetAction(MHS_DEADg2, 1.0f);  // attente genoux
		}

		time = 100.0f;

		dir.x = 0.0f;
		dir.z = 0.0f;
		dir.y = -1.5f*prog;
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.x = 0.0f;
		dir.y = 0.0f;
		dir.z = -(20.0f*PI/180.0f)*prog;
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_DEADg2 )   // mort par balle (genoux) ?
	{
		if ( m_progress >= 1.0f )
		{
			SetAction(MHS_DEADg3, 1.0f);  // plat ventre
		}

		time = 100.0f;

		dir.x = 0.0f;
		dir.z = 0.0f;
		dir.y = -1.5f;
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.x = 0.0f;
		dir.y = 0.0f;
		dir.z = -(20.0f*PI/180.0f);
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_DEADg3 )   // mort par balle (plat ventre) ?
	{
		prog = m_progress;
		if ( prog >= 1.0f )
		{
			prog = 1.0f;

			for ( i=0 ; i<20 ; i++ )
			{
				pos = m_object->RetPosition(0);
				pos.x += (Rand()-0.5f)*8.0f;
				pos.z += (Rand()-0.5f)*8.0f;
				m_terrain->MoveOnFloor(pos);
				speed = D3DVECTOR(0.0f, 0.0f, 0.0f);
				dim.x = 2.0f+Rand()*1.5f;
				dim.y = dim.x;
				m_particule->CreateParticule(pos, speed, dim, PARTICRASH, 2.0f, 0.0f, 0.0f);
			}
			m_sound->Play(SOUND_BOUMv, m_object->RetPosition(0));

			SetAction(MHS_DEADg4, 3.0f);  // attente plat ventre
		}

		time = 100.0f;
		prog = powf(prog, 3.0f);

		dir.y = -(1.5f+1.5f*prog);
		dir.x = 0.0f;
		dir.z = 0.0f;
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.z = -((20.0f*PI/180.0f)+(70.0f*PI/180.0f)*prog);
		dir.x = 0.0f;
		dir.y = 0.0f;
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_DEADg4 )   // mort par balle (plat ventre) ?
	{
		if ( m_progress >= 1.0f )
		{
			m_object->SetEnable(FALSE);
		}

		time = 100.0f;

		dir.y = -(1.5f+1.5f);
		dir.x = 0.0f;
		dir.z = 0.0f;
		actual = m_object->RetLinVibration();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetLinVibration(dir);

		dir.z = -((20.0f*PI/180.0f)+(70.0f*PI/180.0f));
		dir.x = 0.0f;
		dir.y = 0.0f;
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);
	}
	else if ( m_actionType == MHS_DEADw )   // mort noy� ?
	{
		pos = m_object->RetPosition(0);
		level = m_water->RetLevel()-0.5f;
		if ( pos.y < level )
		{
			pos.y += 4.0f*event.rTime;  // remonte � la surface
			if ( pos.y > level )  pos.y = level;
			m_object->SetPosition(0, pos);
		}
		if ( pos.y > level )
		{
			pos.y -= 10.0f*event.rTime;  // descend vite
			if ( pos.y < level )  pos.y = level;
			m_object->SetPosition(0, pos);
		}

		prog = m_progress;
		if ( prog >= 1.0f )
		{
			prog = 1.0f;
			if ( pos.y >= level )  m_object->SetEnable(FALSE);
		}

		prog *= 2.0f;
		if ( prog > 1.0f )  prog = 1.0f;

		time = 100.0f;

		dir.z = -(90.0f*PI/180.0f)*prog;
		dir.x = Rand()*0.3f*deadFactor;
		dir.y = Rand()*0.3f*deadFactor;
		actual = m_object->RetInclinaison();
		dir.x = Smooth(actual.x, dir.x, time);
		dir.y = Smooth(actual.y, dir.y, time);
		dir.z = Smooth(actual.z, dir.z, time);
		m_object->SetInclinaison(dir);

		m_object->SetCirVibration(D3DVECTOR(0.0f, 0.0f, 0.0f));
	}
	else if ( m_actionType == MHS_LOST )   // perdu ?
	{
		time = m_time;
		if ( time < 10.0f )  time *= time/10.0f;  // d�marre lentement

		dir.x = time*2.0f;
		dir.y = sinf(m_time*0.8f)*0.8f;
		dir.z = sinf(m_time*0.6f)*0.5f;
		m_object->SetInclinaison(dir);
		SetInclinaison(dir);

//?		dir.x = -(sinf(time*0.05f+PI*1.5f)+1.0f)*100.0f;
		dir.x = -(powf(Min(time/30.0f), 4.0f))*1000.0f;  // part au loin
		dir.y = 0.0f;
		dir.z = 0.0f;
		m_object->SetLinVibration(dir);
		SetLinVibration(dir);

		mat = m_object->RetWorldMatrix(0);
		pos = D3DVECTOR(0.5f, 3.7f, 0.0f);
		pos.x += (Rand()-0.5f)*1.0f;
		pos.y += (Rand()-0.5f)*1.0f;
		pos.z += (Rand()-0.5f)*1.0f;
		pos = Transform(*mat, pos);
		speed.x = (Rand()-0.5f)*0.5f;
		speed.y = (Rand()-0.5f)*0.5f;
		speed.z = (Rand()-0.5f)*0.5f;
		dim.x = 0.5f+Rand()*0.5f;
		dim.y = dim.x;
		m_particule->CreateParticule(pos, speed, dim, PARTILENS1, 5.0f, 0.0f, 0.0f);
	}
	else if ( m_actionType == MHS_SATCOM )  // regarde le SatCom ?
	{
		SetCirVibration(D3DVECTOR(0.0f, 0.0f, 0.0f));
		SetLinVibration(D3DVECTOR(0.0f, 0.0f, 0.0f));
		SetInclinaison(D3DVECTOR(0.0f, 0.0f, 0.0f));
	}
	else
	{
		if ( m_physics->RetLand() )  // au sol ?
		{
			time = event.rTime*8.0f;
			if ( bSwim )  time *= 0.25f;

			if ( action == MH_MARCH )   // march ?
			{
				dir.x = sinf(Mod(rTime[0]+0.5f, 1.0f)*PI*2.0f)*0.10f;
				dir.y = sinf(Mod(rTime[0]+0.6f, 1.0f)*PI*2.0f)*0.20f;
				s = m_physics->RetLinMotionX(MO_REASPEED)*0.03f;
			}
			else if ( action == MH_MARCHTAKE )   // march-take ?
			{
				dir.x = sinf(Mod(rTime[0]+0.5f, 1.0f)*PI*2.0f)*0.10f;
				dir.y = sinf(Mod(rTime[0]+0.6f, 1.0f)*PI*2.0f)*0.15f;
				s = m_physics->RetLinMotionX(MO_REASPEED)*0.02f;
			}
			else
			{
				dir.x = 0.0f;
				dir.y = 0.0f;
				s = m_physics->RetLinMotionX(MO_REASPEED)*0.03f;
			}

			if ( s < 0.0f )  s *= 0.5f;
			dir.z = -s*0.7f;

			actual = m_object->RetInclinaison();
			dir.x = Smooth(actual.x, dir.x, time);
			dir.y = Smooth(actual.y, dir.y, time);
			dir.z = Smooth(actual.z, dir.z, time);
			if ( bOnBoard )  dir *= 0.3f;
			m_object->SetInclinaison(dir);
			SetInclinaison(dir);

			if ( action == MH_MARCH )   // march ?
			{
				p2.x = 0.0f;
				p2.y = sinf(Mod(rTime[0]+0.5f, 1.0f)*PI*2.0f)*0.5f;
				p2 = RotatePoint(-m_object->RetAngleY(0), p2);
				dir.x = p2.x;
				dir.z = p2.y;
				dir.y = sinf(Mod(rTime[0]*2.0f, 1.0f)*PI*2.0f)*0.3f;
			}
			else if ( action == MH_MARCHTAKE )   // march-take ?
			{
				p2.x = 0.0f;
				p2.y = sinf(Mod(rTime[0]+0.5f, 1.0f)*PI*2.0f)*0.25f;
				p2 = RotatePoint(-m_object->RetAngleY(0), p2);
				dir.x = p2.x;
				dir.z = p2.y;
				dir.y = sinf(Mod(rTime[0]*2.0f, 1.0f)*PI*2.0f)*0.05f-0.3f;
			}
			else
			{
				dir.x = 0.0f;
				dir.z = 0.0f;
				dir.y = 0.0f;
			}

			actual = m_object->RetLinVibration();
			dir.x = Smooth(actual.x, dir.x, time);
			if ( action == MH_MARCHTAKE )  // march-take ?
			{
				dir.y = -hr;
			}
			else
			{
				s = Min(m_armTimeAction, 1.0f);
				dir.y = Smooth(actual.y, dir.y, time)*s;
				dir.y += -hr*(1.0f-s);
			}
			dir.z = Smooth(actual.z, dir.z, time);
			if ( bOnBoard )  dir *= 0.3f;
			m_object->SetLinVibration(dir);

			dir.x = 0.0f;
			dir.z = 0.0f;
			dir.y = 0.0f;
			SetCirVibration(dir);
		}
	}

	// Gestion de la t�te.
	if ( m_actionType == MHS_TAKE ||  // prend ?
		 m_actionType == MHS_FLAG )   // prend ?
	{
		m_object->SetAngleZ(1, Smooth(m_object->RetAngleZ(1), sinf(m_armTimeAbs*1.0f)*0.2f-0.6f, event.rTime*5.0f));
		m_object->SetAngleX(1, sinf(m_armTimeAbs*1.1f)*0.1f);
		m_object->SetAngleY(1, Smooth(m_object->RetAngleY(1), sinf(m_armTimeAbs*1.3f)*0.2f+rot*0.3f, event.rTime*5.0f));
	}
	else if ( m_actionType == MHS_TAKEOTHER ||  // prend ?
			  m_actionType == MHS_TAKEHIGH  )   // prend ?
	{
		m_object->SetAngleZ(1, Smooth(m_object->RetAngleZ(1), sinf(m_armTimeAbs*1.0f)*0.2f-0.3f, event.rTime*5.0f));
		m_object->SetAngleX(1, sinf(m_armTimeAbs*1.1f)*0.1f);
		m_object->SetAngleY(1, Smooth(m_object->RetAngleY(1), sinf(m_armTimeAbs*1.3f)*0.2f+rot*0.3f, event.rTime*5.0f));
	}
	else if ( m_actionType == MHS_WIN )   // gagn� ?
	{
		float	factor = 0.6f+(sinf(m_armTimeAbs*0.5f)*0.40f);
		m_object->SetAngleZ(1, sinf(m_armTimeAbs*5.0f)*0.20f*factor);
		m_object->SetAngleX(1, sinf(m_armTimeAbs*0.6f)*0.10f);
		m_object->SetAngleY(1, sinf(m_armTimeAbs*1.5f)*0.15f);
	}
	else if ( m_actionType == MHS_LOST )   // perdu ?
	{
		float	factor = 0.6f+(sinf(m_armTimeAbs*0.5f)*0.40f);
		m_object->SetAngleZ(1, sinf(m_armTimeAbs*0.6f)*0.10f);
		m_object->SetAngleX(1, sinf(m_armTimeAbs*0.7f)*0.10f);
		m_object->SetAngleY(1, sinf(m_armTimeAbs*3.0f)*0.30f*factor);
	}
	else if ( m_object->RetDead() )  // mort ?
	{
	}
	else
	{
		m_object->SetAngleZ(1, Smooth(m_object->RetAngleZ(1), sinf(m_armTimeAbs*1.0f)*0.2f, event.rTime*5.0f));
		m_object->SetAngleX(1, sinf(m_armTimeAbs*1.1f)*0.1f);
		m_object->SetAngleY(1, Smooth(m_object->RetAngleY(1), sinf(m_armTimeAbs*1.3f)*0.2f+rot*0.3f, event.rTime*5.0f));
	}

	if ( bOnBoard )
	{
		m_object->SetAngleZ(1, 0.0f);
		m_object->SetAngleX(1, 0.0f);
		m_object->SetAngleY(1, 0.0f);
	}

	// Bruitage des pas.
	if ( legAction == MH_MARCH     ||
		 legAction == MH_MARCHTAKE )
	{
		Sound	sound[2];
		float	speed, synchro, volume[2], freq[2], hard, level;

		speed = m_physics->RetLinMotionX(MO_REASPEED);

		if ( m_object->RetFret() == 0 )
		{
			if ( speed > 0.0f )  synchro = 0.21f;  // synchro en avant
			else                 synchro = 0.29f;  // synchro en arri�re
		}
		else
		{
			if ( speed > 0.0f )  synchro = 0.15f;  // synchro en avant
			else                 synchro = 0.35f;  // synchro en arri�re
		}
		time = rTime[1]+synchro;

		if ( Abs(m_lastSoundMarch-time) > 0.4f &&
			 Mod(time, 0.5f) < 0.1f )
		{
			volume[0] = 0.5f;
			freq[0] = 1.0f;
			if ( m_object->RetFret() != 0 )
			{
//?				volume[0] *= 2.0f;
				freq[0] = 0.7f;
			}
			volume[1] = volume[0];
			freq[1] = freq[0];
			sound[0] = SOUND_CLICK;
			sound[1] = SOUND_CLICK;

			pos = m_object->RetPosition(0);

			level = m_water->RetLevel();
			if ( pos.y <= level+3.0f )  // sous l'eau ?
			{
				sound[0] = SOUND_STEPw;
			}
			else
			{
				hard = m_terrain->RetHardness(pos);

				if ( hard >= 0.875 )
				{
					sound[0] = SOUND_STEPm;  // metal
				}
				else
				{
					hard /= 0.875;
					sound[0] = SOUND_STEPs;  // smooth
					sound[1] = SOUND_STEPh;  // hard

					volume[0] *= 1.0f-hard;
					volume[1] *= hard;
					if ( hard < 0.5f )
					{
						volume[0] *= 1.0f+hard*2.0f;
						volume[1] *= 1.0f+hard*2.0f;
					}
					else
					{
						volume[0] *= 3.0f-hard*2.0f;
						volume[1] *= 3.0f-hard*2.0f;
					}
					freq[0] *= 1.0f+hard;
					freq[1] *= 0.5f+hard;
				}
			}

			if ( sound[0] != SOUND_CLICK )
			{
				m_sound->Play(sound[0], pos, volume[0], freq[0]);
			}
			if ( sound[1] != SOUND_CLICK )
			{
				m_sound->Play(sound[1], pos, volume[1], freq[1]);
			}
			m_lastSoundMarch = time;
		}
	}

	if ( legAction == MH_SWIM )
	{
		time = rTime[0]+0.5f;

		if ( Abs(m_lastSoundMarch-time) > 0.9f &&
			 Mod(time, 1.0f) < 0.1f )
		{
			m_sound->Play(SOUND_SWIM, m_object->RetPosition(0), 0.5f);
			m_lastSoundMarch = time;
		}
	}

	m_lastSoundHhh -= event.rTime;
	if ( m_lastSoundHhh <= 0.0f &&
		 m_object->RetSelect()  &&
		 m_object->RetOption() == 0 )  // casque ?
	{
		m_sound->Play(SOUND_HUMAN1, m_object->RetPosition(0), (0.5f+m_tired*0.2f));
		m_lastSoundHhh = (4.0f-m_tired*2.5f)+(4.0f-m_tired*2.5f)*Rand();
	}

	return TRUE;
}


// Gestion du mode d'affichage lors de la personnalisation du perso.

void CMotionHuman::StartDisplayPerso()
{
	m_bDisplayPerso = TRUE;
}

void CMotionHuman::StopDisplayPerso()
{
	m_bDisplayPerso = FALSE;
}


