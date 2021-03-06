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
// * along with this program. If not, see  http://www.gnu.org/licenses/.// motionvehicle.cpp

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
#include "object.h"
#include "physics.h"
#include "brain.h"
#include "camera.h"
#include "modfile.h"
#include "sound.h"
#include "motion.h"
#include "motionvehicle.h"



#define ARM_NEUTRAL_ANGLE1	 110.0f*PI/180.0f
#define ARM_NEUTRAL_ANGLE2	-130.0f*PI/180.0f
#define ARM_NEUTRAL_ANGLE3	 -50.0f*PI/180.0f



// Constructeur de l'objet.

CMotionVehicle::CMotionVehicle(CInstanceManager* iMan, CObject* object)
							  : CMotion(iMan, object)
{
	int		i;

	CMotion::CMotion(iMan, object);

	for ( i=0 ; i<4 ; i++ )
	{
		m_wheelTurn[i] = 0.0f;
	}
	for ( i=0 ; i<3 ; i++ )
	{
		m_flyPaw[i] = 0.0f;
	}
	m_posTrackLeft  = 0.0f;
	m_posTrackRight = 0.0f;
	m_partiReactor  = -1;
	m_armTimeAbs    = 1000.0f;
	m_armMember     = 1000.0f;
	m_canonTime     = 0.0f;
	m_lastTimeCanon = 0.0f;
	m_wheelLastPos   = D3DVECTOR(0.0f, 0.0f, 0.0f);
	m_wheelLastAngle = D3DVECTOR(0.0f, 0.0f, 0.0f);
	m_posKey         = D3DVECTOR(0.0f, 0.0f, 0.0f);
	m_bFlyFix = FALSE;

	m_bTraceDown = FALSE;
	m_traceColor = 1;  // black
	m_traceWidth = 0.5f;
}

// Destructeur de l'objet.

CMotionVehicle::~CMotionVehicle()
{
}


// Supprime un objet.

void CMotionVehicle::DeleteObject(BOOL bAll)
{
	if ( m_partiReactor != -1 )
	{
		m_particule->DeleteParticule(m_partiReactor);
		m_partiReactor = -1;
	}
}


// Cr�e un v�hicule roulant quelconque pos� sur le sol.

BOOL CMotionVehicle::Create(D3DVECTOR pos, float angle, ObjectType type,
							float power)
{
	CModFile*		pModFile;
	CObject*		pPower;
	int				rank, i, j, parent;
	D3DCOLORVALUE	color;
	char			name[50];

	if ( m_engine->RetRestCreate() < 1+5+18+1 )  return FALSE;

	pModFile = new CModFile(m_iMan);

	m_object->SetType(type);

	// Cr�e la base principale.
	rank = m_engine->CreateObject();
	m_engine->SetObjectType(rank, TYPEVEHICULE);  // c'est un objet mobile
	m_object->SetObjectRank(0, rank);

	if ( type == OBJECT_MOBILEfa ||
		 type == OBJECT_MOBILEfc ||
		 type == OBJECT_MOBILEfi ||
		 type == OBJECT_MOBILEfs )
	{
		pModFile->ReadModel("objects\\lem1f.mod");
	}
	if ( type == OBJECT_MOBILEta ||
		 type == OBJECT_MOBILEtc ||
		 type == OBJECT_MOBILEti ||
		 type == OBJECT_MOBILEts )
	{
		pModFile->ReadModel("objects\\lem1t.mod");
	}
	if ( type == OBJECT_MOBILEwa ||
		 type == OBJECT_MOBILEwc ||
		 type == OBJECT_MOBILEwi ||
		 type == OBJECT_MOBILEws )
	{
		if ( m_object->RetTrainer() )
		{
			pModFile->ReadModel("objects\\lem1wt.mod");
		}
		else
		{
			pModFile->ReadModel("objects\\lem1w.mod");
		}
	}
	if ( type == OBJECT_MOBILEia ||
		 type == OBJECT_MOBILEic ||
		 type == OBJECT_MOBILEii ||
		 type == OBJECT_MOBILEis )
	{
		pModFile->ReadModel("objects\\lem1i.mod");
	}
	if ( type == OBJECT_MOBILErt ||
		 type == OBJECT_MOBILErc ||
		 type == OBJECT_MOBILErr ||
		 type == OBJECT_MOBILErs )
	{
		pModFile->ReadModel("objects\\roller1.mod");
	}
	if ( type == OBJECT_MOBILEsa )
	{
		pModFile->ReadModel("objects\\subm1.mod");
	}
	if ( type == OBJECT_MOBILEtg )
	{
		pModFile->ReadModel("objects\\target.mod");
	}
	if ( type == OBJECT_MOBILEwt )
	{
		pModFile->ReadModel("objects\\trainerw.mod");
	}
	if ( type == OBJECT_MOBILEft )
	{
		pModFile->ReadModel("objects\\trainerf.mod");
	}
	if ( type == OBJECT_MOBILEtt )
	{
		pModFile->ReadModel("objects\\trainert.mod");
	}
	if ( type == OBJECT_MOBILEit )
	{
		pModFile->ReadModel("objects\\traineri.mod");
	}
	if ( type == OBJECT_MOBILEdr )
	{
		pModFile->ReadModel("objects\\drawer1.mod");
	}
	if ( type == OBJECT_APOLLO2 )
	{
		pModFile->ReadModel("objects\\apolloj1.mod");
	}
	pModFile->CreateEngineObject(rank);

	m_object->SetPosition(0, pos);
	m_object->SetAngleY(0, angle);

	// Un v�hicule doit avoir obligatoirement une sph�re de
	// collision avec un centre (0;y;0) (voir GetCrashSphere).
	if ( type == OBJECT_MOBILErt ||
		 type == OBJECT_MOBILErc ||
		 type == OBJECT_MOBILErr ||
		 type == OBJECT_MOBILErs )
	{
		m_object->CreateCrashSphere(D3DVECTOR(0.0f, 4.0f, 0.0f), 6.5f, SOUND_BOUMm, 0.45f);
		m_object->SetGlobalSphere(D3DVECTOR(0.0f, 3.0f, 0.0f), 7.0f);
	}
	else if ( type == OBJECT_MOBILEsa )
	{
		m_object->CreateCrashSphere(D3DVECTOR(0.0f, 3.0f, 0.0f), 4.5f, SOUND_BOUMm, 0.45f);
		m_object->SetGlobalSphere(D3DVECTOR(0.0f, 3.0f, 0.0f), 6.0f);
	}
	else if ( type == OBJECT_MOBILEdr )
	{
		m_object->CreateCrashSphere(D3DVECTOR(0.0f, 3.0f, 0.0f), 5.0f, SOUND_BOUMm, 0.45f);
		m_object->SetGlobalSphere(D3DVECTOR(0.0f, 3.0f, 0.0f), 7.0f);
	}
	else if ( type == OBJECT_APOLLO2 )
	{
		m_object->CreateCrashSphere(D3DVECTOR(0.0f, 0.0f, 0.0f), 8.0f, SOUND_BOUMm, 0.45f);
	}
	else
	{
		m_object->CreateCrashSphere(D3DVECTOR(0.0f, 3.0f, 0.0f), 4.5f, SOUND_BOUMm, 0.45f);
		m_object->SetGlobalSphere(D3DVECTOR(0.0f, 4.0f, 0.0f), 6.0f);
	}

	if ( type == OBJECT_MOBILEfa ||
		 type == OBJECT_MOBILEta ||
		 type == OBJECT_MOBILEwa ||
		 type == OBJECT_MOBILEia )
	{
		// Cr�e le bras.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\lem2.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(0.0f, 5.3f, 0.0f));
		m_object->SetAngleZ(1, ARM_NEUTRAL_ANGLE1);

		// Cr�e l'avant-bras.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 1);
		pModFile->ReadModel("objects\\lem3.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(5.0f, 0.0f, 0.0f));
		m_object->SetAngleZ(2, ARM_NEUTRAL_ANGLE2);

		// Cr�e la main.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(3, rank);
		m_object->SetObjectParent(3, 2);
		pModFile->ReadModel("objects\\lem4.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(3, D3DVECTOR(3.5f, 0.0f, 0.0f));
		m_object->SetAngleZ(3, ARM_NEUTRAL_ANGLE3);
		m_object->SetAngleX(3, PI/2.0f);

		// Cr�e la pince proche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(4, rank);
		m_object->SetObjectParent(4, 3);
		pModFile->ReadModel("objects\\lem5.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(4, D3DVECTOR(1.5f, 0.0f, 0.0f));
		m_object->SetAngleZ(4, -PI*0.10f);

		// Cr�e la pince �loign�e.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(5, rank);
		m_object->SetObjectParent(5, 3);
		pModFile->ReadModel("objects\\lem6.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(5, D3DVECTOR(1.5f, 0.0f, 0.0f));
		m_object->SetAngleZ(5, PI*0.10f);
	}

	if ( type == OBJECT_MOBILEfs ||
		 type == OBJECT_MOBILEts ||
		 type == OBJECT_MOBILEws ||
		 type == OBJECT_MOBILEis )
	{
		// Cr�e le bras.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\lem2.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(0.0f, 5.3f, 0.0f));
		m_object->SetAngleZ(1, 110.0f*PI/180.0f);

		// Cr�e l'avant-bras.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 1);
		pModFile->ReadModel("objects\\lem3.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(5.0f, 0.0f, 0.0f));
		m_object->SetAngleZ(2, -110.0f*PI/180.0f);

		// Cr�e le capteur.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(3, rank);
		m_object->SetObjectParent(3, 2);
		pModFile->ReadModel("objects\\lem4s.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(3, D3DVECTOR(3.5f, 0.0f, 0.0f));
		m_object->SetAngleZ(3, -65.0f*PI/180.0f);
	}

	if ( type == OBJECT_MOBILEfc ||
		 type == OBJECT_MOBILEtc ||
		 type == OBJECT_MOBILEwc ||
		 type == OBJECT_MOBILEic )
	{
		// Cr�e le canon.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\canon.mod");
		pModFile->CreateEngineObject(rank);
//?		m_object->SetPosition(1, D3DVECTOR(0.0f, 5.3f, 0.0f));
		m_object->SetPosition(1, D3DVECTOR(0.0f, 5.3f, 0.0f));
		m_object->SetAngleZ(1, 0.0f);
	}

	if ( type == OBJECT_MOBILEfi ||
		 type == OBJECT_MOBILEti ||
		 type == OBJECT_MOBILEwi ||
		 type == OBJECT_MOBILEii )
	{
		// Cr�e le canon insecte.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\canoni1.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(0.0f, 5.3f, 0.0f));
		m_object->SetAngleZ(1, 0.0f);

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 1);
		pModFile->ReadModel("objects\\canoni2.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(0.0f, 2.5f, 0.0f));
		m_object->SetAngleZ(2, 0.0f);
	}

	if ( type == OBJECT_MOBILEwa ||
		 type == OBJECT_MOBILEwc ||
		 type == OBJECT_MOBILEws ||
		 type == OBJECT_MOBILEwi ||
		 type == OBJECT_MOBILEwt )
	{
		// Cr�e la roue arri�re-droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(-3.0f, 1.0f, -3.0f));

		// Cr�e la roue arri�re-gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(-3.0f, 1.0f, 3.0f));
		m_object->SetAngleY(7, PI);

		// Cr�e la roue avant-droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(8, rank);
		m_object->SetObjectParent(8, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(8, D3DVECTOR(2.0f, 1.0f, -3.0f));

		// Cr�e la roue avant-gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(9, rank);
		m_object->SetObjectParent(9, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(9, D3DVECTOR(2.0f, 1.0f, 3.0f));
		m_object->SetAngleY(9, PI);
	}

	if ( type == OBJECT_MOBILEtg )
	{
		// Cr�e la roue arri�re-droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(-2.0f, 1.0f, -3.0f));

		// Cr�e la roue arri�re-gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(-2.0f, 1.0f, 3.0f));
		m_object->SetAngleY(7, PI);

		// Cr�e la roue avant-droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(8, rank);
		m_object->SetObjectParent(8, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(8, D3DVECTOR(3.0f, 1.0f, -3.0f));

		// Cr�e la roue avant-gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(9, rank);
		m_object->SetObjectParent(9, 0);
		pModFile->ReadModel("objects\\lem2w.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(9, D3DVECTOR(3.0f, 1.0f, 3.0f));
		m_object->SetAngleY(9, PI);
	}

	if ( type == OBJECT_MOBILEta ||
		 type == OBJECT_MOBILEtc ||
		 type == OBJECT_MOBILEti ||
		 type == OBJECT_MOBILEts )  // chenilles ?
	{
		// Cr�e la chenille droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\lem2t.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(0.0f, 2.0f, -3.0f));

		// Cr�e la chenille gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\lem3t.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(0.0f, 2.0f, 3.0f));
	}

	if ( type == OBJECT_MOBILErt ||
		 type == OBJECT_MOBILErc ||
		 type == OBJECT_MOBILErr ||
		 type == OBJECT_MOBILErs )  // grosses chenilles ?
	{
		// Cr�e la chenille droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\roller2.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(0.0f, 2.0f, -3.0f));

		// Cr�e la chenille gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\roller3.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(0.0f, 2.0f, 3.0f));
	}

	if ( type == OBJECT_MOBILEsa )  // chenilles sous-marin ?
	{
		// Cr�e la chenille droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\subm4.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(0.0f, 1.0f, -3.0f));

		// Cr�e la chenille gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\subm5.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(0.0f, 1.0f, 3.0f));
	}

	if ( type == OBJECT_MOBILEdr )  // chenilles ?
	{
		// Cr�e la chenille droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\drawer2.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(0.0f, 1.0f, -3.0f));

		// Cr�e la chenille gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\drawer3.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(0.0f, 1.0f, 3.0f));
	}

	if ( type == OBJECT_MOBILEfa ||
		 type == OBJECT_MOBILEfc ||
		 type == OBJECT_MOBILEfs ||
		 type == OBJECT_MOBILEfi ||
		 type == OBJECT_MOBILEft )  // volant ?
	{
		// Cr�e le pied avant.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\lem2f.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(1.7f, 3.0f, 0.0f));

		// Cr�e le pied arri�re droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\lem2f.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(-1.8f, 3.0f, -1.5f));
		m_object->SetAngleY(7, 120.0f*PI/180.0f);

		// Cr�e le pied arri�re gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(8, rank);
		m_object->SetObjectParent(8, 0);
		pModFile->ReadModel("objects\\lem2f.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(8, D3DVECTOR(-1.8f, 3.0f, 1.5f));
		m_object->SetAngleY(8, -120.0f*PI/180.0f);
	}

	if ( type == OBJECT_MOBILEia ||
		 type == OBJECT_MOBILEic ||
		 type == OBJECT_MOBILEis ||
		 type == OBJECT_MOBILEii )  // pattes d'insecte ?
	{
		float		table[] =
		{
		//	  x		  y		  z
			-1.5f,	 1.2f,	-0.7f,	// patte arri�re
			 0.0f,	 0.0f,	-1.0f,
			 0.0f,	 0.0f,	-2.0f,

			 0.0f,	 1.2f,	-0.9f,	// patte milieu
			 0.0f,	 0.0f,	-1.0f,
			 0.0f,	 0.0f,	-2.0f,

			 1.5f,	 1.2f,	-0.7f,	// patte avant
			 0.0f,	 0.0f,	-1.0f,
			 0.0f,	 0.0f,	-2.0f,
		};

		for ( i=0 ; i<3 ; i++ )
		{
			for ( j=0 ; j<3 ; j++ )
			{
				sprintf(name, "objects\\ant%d.mod", j+4);  // 4..6

				// Cr�e la patte droite.
				rank = m_engine->CreateObject();
				m_engine->SetObjectType(rank, TYPEDESCENDANT);
				m_object->SetObjectRank(6+i*3+j, rank);
				if ( j == 0 )  parent = 0;
				else           parent = 6+i*3+j-1;
				m_object->SetObjectParent(6+i*3+j, parent);
				pModFile->ReadModel(name);
				pModFile->CreateEngineObject(rank);
				pos.x = table[i*9+j*3+0];
				pos.y = table[i*9+j*3+1];
				pos.z = table[i*9+j*3+2];
				m_object->SetPosition(6+i*3+j, pos);

				// Cr�e la patte gauche.
				rank = m_engine->CreateObject();
				m_engine->SetObjectType(rank, TYPEDESCENDANT);
				m_object->SetObjectRank(15+i*3+j, rank);
				if ( j == 0 )  parent = 0;
				else           parent = 15+i*3+j-1;
				m_object->SetObjectParent(15+i*3+j, parent);
				pModFile->ReadModel(name);
				pModFile->Mirror();
				pModFile->CreateEngineObject(rank);
				pos.x =  table[i*9+j*3+0];
				pos.y =  table[i*9+j*3+1];
				pos.z = -table[i*9+j*3+2];
				m_object->SetPosition(15+i*3+j, pos);
			}
		}
	}

	if ( type == OBJECT_MOBILErt )
	{
		// Cr�e le support.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\roller2t.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(0.0f, 0.0f, 0.0f));
		m_object->SetAngleZ(1, 0.0f);

		// Cr�e le pilon.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 0);
		pModFile->ReadModel("objects\\roller3t.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(9.0f, 4.0f, 0.0f));
		m_object->SetAngleZ(2, 0.0f);
	}

	if ( type == OBJECT_MOBILErc )
	{
		// Cr�e le support.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\roller2c.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(3.0f, 4.6f, 0.0f));
		m_object->SetAngleZ(1, PI/8.0f);

		// Cr�e le canon.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 0);
		pModFile->ReadModel("objects\\roller3p.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(7.0f, 6.5f, 0.0f));
		m_object->SetAngleZ(2, 0.0f);
	}

	if ( type == OBJECT_MOBILErr )
	{
		// Cr�e le support.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\recover1.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(2.0f, 5.0f, 0.0f));

		// Cr�e le bras droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 1);
		pModFile->ReadModel("objects\\recover2.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(0.1f, 0.0f, -5.0f));
		m_object->SetAngleZ(2, 126.0f*PI/180.0f);

		// Cr�e l'avant-bras droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(3, rank);
		m_object->SetObjectParent(3, 2);
		pModFile->ReadModel("objects\\recover3.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(3, D3DVECTOR(5.0f, 0.0f, -0.5f));
		m_object->SetAngleZ(3, -144.0f*PI/180.0f);

		// Cr�e le bras gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(4, rank);
		m_object->SetObjectParent(4, 1);
		pModFile->ReadModel("objects\\recover2.mod");
		pModFile->Mirror();
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(4, D3DVECTOR(0.1f, 0.0f, 5.0f));
		m_object->SetAngleZ(4, 126.0f*PI/180.0f);

		// Cr�e l'avant-bras gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(5, rank);
		m_object->SetObjectParent(5, 4);
		pModFile->ReadModel("objects\\recover3.mod");
		pModFile->Mirror();
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(5, D3DVECTOR(5.0f, 0.0f, 0.5f));
		m_object->SetAngleZ(5, -144.0f*PI/180.0f);
	}

	if ( type == OBJECT_MOBILErs )
	{
		// Cr�e le support.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\roller2s.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(0.0f, 0.0f, 0.0f));
		m_object->SetAngleZ(1, 0.0f);

		// Cr�e le piston interm�diaire.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 1);
		pModFile->ReadModel("objects\\roller3s.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(7.0f, 4.5f, 0.0f));
		m_object->SetAngleZ(2, 0.0f);

		// Cr�e le piston avec la sph�re.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(3, rank);
		m_object->SetObjectParent(3, 2);
		pModFile->ReadModel("objects\\roller4s.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(3, D3DVECTOR(0.0f, 1.0f, 0.0f));
		m_object->SetAngleZ(3, 0.0f);
	}

	if ( type == OBJECT_MOBILEsa )
	{
		// Cr�e le support.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\subm2.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(4.2f, 3.0f, 0.0f));

		// Cr�e la pince droite.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 1);
		pModFile->ReadModel("objects\\subm3.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(0.5f, 0.0f, -1.5f));

		// Cr�e la pince gauche.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(3, rank);
		m_object->SetObjectParent(3, 1);
		pModFile->ReadModel("objects\\subm3.mod");
		pModFile->Mirror();
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(3, D3DVECTOR(0.5f, 0.0f, 1.5f));
	}

	if ( type == OBJECT_MOBILEdr )
	{
		// Cr�e le carousel.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\drawer4.mod");
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(-3.0f, 3.0f, 0.0f));

		// Cr�e la cl�.
		if ( m_object->RetToy() )
		{
			rank = m_engine->CreateObject();
			m_engine->SetObjectType(rank, TYPEDESCENDANT);
			m_object->SetObjectRank(2, rank);
			m_object->SetObjectParent(2, 0);
			pModFile->ReadModel("objects\\drawer5.mod");
			pModFile->CreateEngineObject(rank);
			m_posKey = D3DVECTOR(3.0f, 5.7f, 0.0f);
			m_object->SetPosition(2, m_posKey);
			m_object->SetAngleY(2, 90.0f*PI/180.0f);
		}

		// Cr�e les crayons.
		for ( i=0 ; i<8 ; i++ )
		{
			rank = m_engine->CreateObject();
			m_engine->SetObjectType(rank, TYPEDESCENDANT);
			m_object->SetObjectRank(10+i, rank);
			m_object->SetObjectParent(10+i, 1);
			sprintf(name, "objects\\drawer%d.mod", 10+i);
			pModFile->ReadModel(name);
			pModFile->CreateEngineObject(rank);
			m_object->SetPosition(10+i, D3DVECTOR(0.0f, 0.0f, 0.0f));
			m_object->SetAngleY(10+i, 45.0f*PI/180.0f*i);
		}
	}

	if ( type == OBJECT_MOBILEwt )
	{
		// Cr�e la cl�.
		if ( m_object->RetToy() )
		{
			rank = m_engine->CreateObject();
			m_engine->SetObjectType(rank, TYPEDESCENDANT);
			m_object->SetObjectRank(2, rank);
			m_object->SetObjectParent(2, 0);
			pModFile->ReadModel("objects\\drawer5.mod");
			pModFile->CreateEngineObject(rank);
			m_posKey = D3DVECTOR(0.2f, 4.1f, 0.0f);
			m_object->SetPosition(2, m_posKey);
			m_object->SetAngleY(2, 90.0f*PI/180.0f);
		}
	}

	if ( type == OBJECT_APOLLO2 )
	{
		// Cr�e les accessoirs.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(1, rank);
		m_object->SetObjectParent(1, 0);
		pModFile->ReadModel("objects\\apolloj2.mod");  // antenne
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(1, D3DVECTOR(5.5f, 8.8f, 2.0f));
		m_object->SetAngleY(1, -120.0f*PI/180.0f);
		m_object->SetAngleZ(1,   45.0f*PI/180.0f);

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(2, rank);
		m_object->SetObjectParent(2, 0);
		pModFile->ReadModel("objects\\apolloj3.mod");  // cam�ra
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(2, D3DVECTOR(5.5f, 2.8f, -2.0f));
		m_object->SetAngleY(2, 30.0f*PI/180.0f);

		// Cr�e les roues.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(6, rank);
		m_object->SetObjectParent(6, 0);
		pModFile->ReadModel("objects\\apolloj4.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(6, D3DVECTOR(-5.75f, 1.65f, -5.0f));

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(7, rank);
		m_object->SetObjectParent(7, 0);
		pModFile->ReadModel("objects\\apolloj4.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(7, D3DVECTOR(-5.75f, 1.65f, 5.0f));

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(8, rank);
		m_object->SetObjectParent(8, 0);
		pModFile->ReadModel("objects\\apolloj4.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(8, D3DVECTOR(5.75f, 1.65f, -5.0f));

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(9, rank);
		m_object->SetObjectParent(9, 0);
		pModFile->ReadModel("objects\\apolloj4.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(9, D3DVECTOR(5.75f, 1.65f, 5.00f));

		// Cr�e les gardes boues.
		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(10, rank);
		m_object->SetObjectParent(10, 0);
		pModFile->ReadModel("objects\\apolloj6.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(10, D3DVECTOR(-5.75f, 1.65f, -5.0f));

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(11, rank);
		m_object->SetObjectParent(11, 0);
		pModFile->ReadModel("objects\\apolloj6.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(11, D3DVECTOR(-5.75f, 1.65f, 5.0f));

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(12, rank);
		m_object->SetObjectParent(12, 0);
		pModFile->ReadModel("objects\\apolloj5.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(12, D3DVECTOR(5.75f, 1.65f, -5.0f));

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEDESCENDANT);
		m_object->SetObjectRank(13, rank);
		m_object->SetObjectParent(13, 0);
		pModFile->ReadModel("objects\\apolloj5.mod");  // roue
		pModFile->CreateEngineObject(rank);
		m_object->SetPosition(13, D3DVECTOR(5.75f, 1.65f, 5.00f));
	}

#if 1
	if ( type == OBJECT_MOBILErt ||
		 type == OBJECT_MOBILErc ||
		 type == OBJECT_MOBILErr ||
		 type == OBJECT_MOBILErs )
	{
		m_object->CreateShadowCircle(6.0f, 1.0f);
	}
	else if ( type == OBJECT_MOBILEta ||
			  type == OBJECT_MOBILEtc ||
			  type == OBJECT_MOBILEti ||
			  type == OBJECT_MOBILEts ||
			  type == OBJECT_MOBILEsa )
	{
		m_object->CreateShadowCircle(5.0f, 1.0f);
	}
	else if ( type == OBJECT_MOBILEdr )
	{
		m_object->CreateShadowCircle(4.5f, 1.0f);
	}
	else if ( type == OBJECT_APOLLO2 )
	{
		m_object->CreateShadowCircle(7.0f, 0.8f);
	}
	else
	{
		m_object->CreateShadowCircle(4.0f, 1.0f);
	}
#else
	if ( type == OBJECT_MOBILErt ||
		 type == OBJECT_MOBILErc ||
		 type == OBJECT_MOBILErr ||
		 type == OBJECT_MOBILErs )
	{
		m_object->CreateShadowCircle(6.0f, 1.0f, D3DSHADOWTANK);
	}
	else if ( type == OBJECT_MOBILEta ||
			  type == OBJECT_MOBILEtc ||
			  type == OBJECT_MOBILEti ||
			  type == OBJECT_MOBILEts )
	{
		m_object->CreateShadowCircle(4.0f, 1.0f, D3DSHADOWTANK);
	}
	else if ( type == OBJECT_MOBILEfa ||
			  type == OBJECT_MOBILEfc ||
			  type == OBJECT_MOBILEfi ||
			  type == OBJECT_MOBILEfs )
	{
		m_object->CreateShadowCircle(4.0f, 1.0f, D3DSHADOWFLY);
	}
	else if ( type == OBJECT_MOBILEwa ||
			  type == OBJECT_MOBILEwc ||
			  type == OBJECT_MOBILEwi ||
			  type == OBJECT_MOBILEws )
	{
		m_object->CreateShadowCircle(4.0f, 1.0f, D3DSHADOWWHEEL);
	}
	else if ( type == OBJECT_APOLLO2 )
	{
		m_object->CreateShadowCircle(6.0f, 0.8f);
	}
	else
	{
		m_object->CreateShadowCircle(4.0f, 1.0f, D3DSHADOWNORM);
	}
#endif

	if ( type == OBJECT_MOBILEfa ||
		 type == OBJECT_MOBILEfc ||
		 type == OBJECT_MOBILEfi ||
		 type == OBJECT_MOBILEfs ||
		 type == OBJECT_MOBILEft )  // volant ?
	{
//?		color.r = 0.5f-1.0f;
//?		color.g = 0.2f-1.0f;
//?		color.b = 0.0f-1.0f;  // orange
//?		color.r = 0.8f;
//?		color.g = 0.6f;
//?		color.b = 0.0f;  // jaune-orange
		color.r = 0.0f;
		color.g = 0.4f;
		color.b = 0.8f;  // bleu
		color.a = 0.0f;
		m_object->CreateShadowLight(50.0f, color);
	}

	CreatePhysics(type);
	m_object->SetFloorHeight(0.0f);

	if ( power > 0.0f            &&
		 type != OBJECT_MOBILEdr &&
		 type != OBJECT_APOLLO2  )
	{
		color.r = 1.0f;
		color.g = 1.0f;
		color.b = 0.0f;  // jaune
		color.a = 0.0f;
		m_object->CreateEffectLight(20.0f, color);

		// Cr�e la pile.
		pPower = new CObject(m_iMan);
		pPower->SetType(power<=1.0f?OBJECT_POWER:OBJECT_ATOMIC);

		rank = m_engine->CreateObject();
		m_engine->SetObjectType(rank, TYPEFIX);
		pPower->SetObjectRank(0, rank);

		if ( power <= 1.0f )  pModFile->ReadModel("objects\\power.mod");
		else                  pModFile->ReadModel("objects\\atomic.mod");
		pModFile->CreateEngineObject(rank);

		pPower->SetPosition(0, m_object->RetCharacter()->posPower);
		pPower->CreateCrashSphere(D3DVECTOR(0.0f, 1.0f, 0.0f), 1.0f, SOUND_BOUMm, 0.45f);
		pPower->SetGlobalSphere(D3DVECTOR(0.0f, 1.0f, 0.0f), 1.5f);

		pPower->SetTruck(m_object);
		m_object->SetPower(pPower);

		if ( power <= 1.0f )  pPower->SetEnergy(power);
		else                  pPower->SetEnergy(power/100.0f);
	}

	pos = m_object->RetPosition(0);
	m_object->SetPosition(0, pos);  // pour afficher les ombres tout de suite

	m_engine->LoadAllTexture();

	delete pModFile;
	return TRUE;
}

// Cr�e la physique de l'objet.

void CMotionVehicle::CreatePhysics(ObjectType type)
{
	Character*	character;

	character = m_object->RetCharacter();

	if ( type == OBJECT_MOBILEwa ||
		 type == OBJECT_MOBILEwc ||
		 type == OBJECT_MOBILEwi ||
		 type == OBJECT_MOBILEws ||
		 type == OBJECT_MOBILEwt )  // roues ?
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 3.0f;
		character->wheelBack  = 4.0f;
		character->wheelLeft  = 4.0f;
		character->wheelRight = 4.0f;
		character->posPower   = D3DVECTOR(-3.2f, 3.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 20.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 10.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 50.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 30.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 20.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.8f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.8f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  8.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  8.0f);
		m_physics->SetCirMotionY(MO_STOACCEL, 12.0f);
	}

	if ( type == OBJECT_MOBILEtg )
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 4.0f;
		character->wheelBack  = 3.0f;
		character->wheelLeft  = 4.0f;
		character->wheelRight = 4.0f;
		character->posPower   = D3DVECTOR(-3.2f, 3.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 20.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 10.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 50.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 20.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 20.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.8f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.8f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL, 10.0f);
		m_physics->SetCirMotionY(MO_RECACCEL, 10.0f);
		m_physics->SetCirMotionY(MO_STOACCEL, 15.0f);
	}

	if ( type == OBJECT_MOBILEta ||
		 type == OBJECT_MOBILEtc ||
		 type == OBJECT_MOBILEti ||
		 type == OBJECT_MOBILEts )  // chenilles ?
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 4.0f;
		character->wheelBack  = 4.0f;
		character->wheelLeft  = 4.8f;
		character->wheelRight = 4.8f;
		character->posPower   = D3DVECTOR(-3.2f, 3.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 15.0f);
		m_physics->SetLinMotionX(MO_RECSPEED,  8.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 15.0f);
		m_physics->SetLinMotionX(MO_RECACCEL,  8.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 20.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 10.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 40.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL, 10.0f);
		m_physics->SetCirMotionY(MO_RECACCEL, 10.0f);
		m_physics->SetCirMotionY(MO_STOACCEL,  6.0f);
	}

	if ( type == OBJECT_MOBILEia ||
		 type == OBJECT_MOBILEic ||
		 type == OBJECT_MOBILEii ||
		 type == OBJECT_MOBILEis )  // pattes ?
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 4.0f;
		character->wheelBack  = 4.0f;
		character->wheelLeft  = 5.0f;
		character->wheelRight = 5.0f;
		character->posPower   = D3DVECTOR(-3.2f, 3.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 15.0f);
		m_physics->SetLinMotionX(MO_RECSPEED,  8.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 10.0f);
//?		m_physics->SetLinMotionX(MO_TERFORCE, 15.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 10.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 40.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL, 10.0f);
		m_physics->SetCirMotionY(MO_RECACCEL, 10.0f);
		m_physics->SetCirMotionY(MO_STOACCEL, 15.0f);
	}

	if ( type == OBJECT_MOBILEfa ||
		 type == OBJECT_MOBILEfc ||
		 type == OBJECT_MOBILEfi ||
		 type == OBJECT_MOBILEfs ||
		 type == OBJECT_MOBILEft )  // volant ?
	{
		m_physics->SetType(TYPE_FLYING);

		character->wheelFront = 5.0f;
		character->wheelBack  = 4.0f;
		character->wheelLeft  = 4.5f;
		character->wheelRight = 4.5f;
		character->posPower   = D3DVECTOR(-3.2f, 3.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 50.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 50.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 20.0f);
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

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.4f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.4f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  2.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  2.0f);
		m_physics->SetCirMotionY(MO_STOACCEL,  2.0f);
	}

	if ( type == OBJECT_MOBILErt ||
		 type == OBJECT_MOBILErc ||
		 type == OBJECT_MOBILErr ||
		 type == OBJECT_MOBILErs )  // grosses chenilles ?
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 5.0f;
		character->wheelBack  = 5.0f;
		character->wheelLeft  = 6.0f;
		character->wheelRight = 6.0f;
		character->posPower   = D3DVECTOR(-5.8f, 4.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 10.0f);
		m_physics->SetLinMotionX(MO_RECSPEED,  5.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 10.0f);
		m_physics->SetLinMotionX(MO_RECACCEL,  5.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 20.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 10.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 40.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.3f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.3f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  2.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  2.0f);
		m_physics->SetCirMotionY(MO_STOACCEL,  4.0f);
	}

	if ( type == OBJECT_MOBILEsa )
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 4.0f;
		character->wheelBack  = 4.0f;
		character->wheelLeft  = 4.0f;
		character->wheelRight = 4.0f;
		character->posPower   = D3DVECTOR(-5.0f, 3.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 15.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 10.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 10.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 20.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 10.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 40.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  5.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  5.0f);
		m_physics->SetCirMotionY(MO_STOACCEL, 10.0f);
	}

	if ( type == OBJECT_MOBILEdr )
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 4.0f;
		character->wheelBack  = 4.0f;
		character->wheelLeft  = 4.0f;
		character->wheelRight = 4.0f;
		character->posPower   = D3DVECTOR(-5.0f, 3.0f, 0.0f);

		m_physics->SetLinMotionX(MO_ADVSPEED, 15.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 10.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 10.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  5.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 20.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 10.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 40.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.5f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  5.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  5.0f);
		m_physics->SetCirMotionY(MO_STOACCEL, 10.0f);
	}

	if ( type == OBJECT_APOLLO2 )  // jeep ?
	{
		m_physics->SetType(TYPE_ROLLING);

		character->wheelFront = 6.0f;
		character->wheelBack  = 6.0f;
		character->wheelLeft  = 5.0f;
		character->wheelRight = 5.0f;

		m_physics->SetLinMotionX(MO_ADVSPEED, 15.0f);
		m_physics->SetLinMotionX(MO_RECSPEED, 10.0f);
		m_physics->SetLinMotionX(MO_ADVACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_RECACCEL, 20.0f);
		m_physics->SetLinMotionX(MO_STOACCEL, 40.0f);
		m_physics->SetLinMotionX(MO_TERSLIDE,  2.0f);
		m_physics->SetLinMotionZ(MO_TERSLIDE,  2.0f);
		m_physics->SetLinMotionX(MO_TERFORCE, 30.0f);
		m_physics->SetLinMotionZ(MO_TERFORCE, 10.0f);
		m_physics->SetLinMotionZ(MO_MOTACCEL, 20.0f);

		m_physics->SetCirMotionY(MO_ADVSPEED,  0.4f*PI);
		m_physics->SetCirMotionY(MO_RECSPEED,  0.4f*PI);
		m_physics->SetCirMotionY(MO_ADVACCEL,  2.0f);
		m_physics->SetCirMotionY(MO_RECACCEL,  2.0f);
		m_physics->SetCirMotionY(MO_STOACCEL,  4.0f);
	}
}


// Gestion d'un �v�nement.

BOOL CMotionVehicle::EventProcess(const Event &event)
{
	CMotion::EventProcess(event);

	if ( event.event == EVENT_FRAME )
	{
		return EventFrame(event);
	}

	if ( event.event == EVENT_KEYDOWN )
	{
	}

	return TRUE;
}

// Gestion d'un �v�nement.

BOOL CMotionVehicle::EventFrame(const Event &event)
{
	D3DMATRIX*	mat;
	Character*	character;
	D3DVECTOR	pos, angle, floor;
	ObjectType	type;
	float		s, a, speedBL, speedBR, speedFL, speedFR, h, a1, a2;
	float		back, front, dist, radius, limit[2];

	if ( m_engine->RetPause() )  return TRUE;
	if ( !m_engine->IsVisiblePoint(m_object->RetPosition(0)) )  return TRUE;

	type = m_object->RetType();

	if ( type == OBJECT_MOBILEwa ||
		 type == OBJECT_MOBILEwc ||
		 type == OBJECT_MOBILEwi ||
		 type == OBJECT_MOBILEws ||
		 type == OBJECT_MOBILEwt ||
		 type == OBJECT_MOBILEtg ||
		 type == OBJECT_APOLLO2  )  // roues ?
	{
		s = m_physics->RetLinMotionX(MO_MOTSPEED)*1.0f;
		a = m_physics->RetCirMotionY(MO_MOTSPEED)*3.0f;

		if ( type == OBJECT_APOLLO2 )  s *= 0.5f;

		speedBR = -s+a;
		speedBL =  s+a;
		speedFR = -s+a;
		speedFL =  s+a;

		m_object->SetAngleZ(6, m_object->RetAngleZ(6)+event.rTime*speedBR);  // tourne les roues
		m_object->SetAngleZ(7, m_object->RetAngleZ(7)+event.rTime*speedBL);
		m_object->SetAngleZ(8, m_object->RetAngleZ(8)+event.rTime*speedFR);
		m_object->SetAngleZ(9, m_object->RetAngleZ(9)+event.rTime*speedFL);

		if ( s > 0.0f )
		{
			m_wheelTurn[0] = -a*0.05f;
			m_wheelTurn[1] = -a*0.05f+PI;
			m_wheelTurn[2] =  a*0.05f;
			m_wheelTurn[3] =  a*0.05f+PI;
		}
		else if ( s < 0.0f )
		{
			m_wheelTurn[0] =  a*0.05f;
			m_wheelTurn[1] =  a*0.05f+PI;
			m_wheelTurn[2] = -a*0.05f;
			m_wheelTurn[3] = -a*0.05f+PI;
		}
		else
		{
			m_wheelTurn[0] =  Abs(a)*0.05f;
			m_wheelTurn[1] = -Abs(a)*0.05f+PI;
			m_wheelTurn[2] = -Abs(a)*0.05f;
			m_wheelTurn[3] =  Abs(a)*0.05f+PI;
		}
		m_object->SetAngleY(6, m_object->RetAngleY(6)+(m_wheelTurn[0]-m_object->RetAngleY(6))*event.rTime*8.0f);
		m_object->SetAngleY(7, m_object->RetAngleY(7)+(m_wheelTurn[1]-m_object->RetAngleY(7))*event.rTime*8.0f);
		m_object->SetAngleY(8, m_object->RetAngleY(8)+(m_wheelTurn[2]-m_object->RetAngleY(8))*event.rTime*8.0f);
		m_object->SetAngleY(9, m_object->RetAngleY(9)+(m_wheelTurn[3]-m_object->RetAngleY(9))*event.rTime*8.0f);

		if ( type == OBJECT_APOLLO2 )
		{
			m_object->SetAngleY(10, m_object->RetAngleY(6)+(m_wheelTurn[0]-m_object->RetAngleY(6))*event.rTime*8.0f);
			m_object->SetAngleY(11, m_object->RetAngleY(7)+(m_wheelTurn[1]-m_object->RetAngleY(7))*event.rTime*8.0f+PI);
			m_object->SetAngleY(12, m_object->RetAngleY(8)+(m_wheelTurn[2]-m_object->RetAngleY(8))*event.rTime*8.0f);
			m_object->SetAngleY(13, m_object->RetAngleY(9)+(m_wheelTurn[3]-m_object->RetAngleY(9))*event.rTime*8.0f+PI);
		}

		pos = m_object->RetPosition(0);
		angle = m_object->RetAngle(0);
		if ( pos.x   != m_wheelLastPos.x   ||
			 pos.y   != m_wheelLastPos.y   ||
			 pos.z   != m_wheelLastPos.z   ||
			 angle.x != m_wheelLastAngle.x ||
			 angle.y != m_wheelLastAngle.y ||
			 angle.z != m_wheelLastAngle.z )
		{
			m_wheelLastPos = pos;
			m_wheelLastAngle = angle;

			if ( type == OBJECT_MOBILEtg )
			{
				back   = -2.0f;  // position roues arri�res
				front  =  3.0f;  // position roues avants
				dist   =  3.0f;  // �loignement roues Z
				radius =  1.0f;
			}
			else if ( type == OBJECT_APOLLO2 )
			{
				back   = -5.75f;  // position roues arri�res
				front  =  5.75f;  // position roues avants
				dist   =  5.00f;  // �loignement roues Z
				radius =  1.65f;
			}
			else
			{
				back   = -3.0f;  // position roues arri�res
				front  =  2.0f;  // position roues avants
				dist   =  3.0f;  // �loignement roues Z
				radius =  1.0f;
			}

			if ( Length(pos, m_engine->RetEyePt()) < 50.0f )  // suspension ?
			{
				character = m_object->RetCharacter();
				mat = m_object->RetWorldMatrix(0);

				pos.x = -character->wheelBack;  // roue arri�re droite
				pos.z = -character->wheelRight;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				h = m_terrain->RetFloorHeight(pos);
				if ( h >  0.5f )  h =  0.5f;
				if ( h < -0.5f )  h = -0.5f;
				pos.x =  back;
				pos.y =  radius-h;
				pos.z = -dist;
				m_object->SetPosition(6, pos);
				if ( type == OBJECT_APOLLO2 )  m_object->SetPosition(10, pos);

				pos.x = -character->wheelBack;  // roue arri�re gauche
				pos.z =  character->wheelLeft;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				h = m_terrain->RetFloorHeight(pos);
				if ( h >  0.5f )  h =  0.5f;
				if ( h < -0.5f )  h = -0.5f;
				pos.x =  back;
				pos.y =  radius-h;
				pos.z =  dist;
				m_object->SetPosition(7, pos);
				if ( type == OBJECT_APOLLO2 )  m_object->SetPosition(11, pos);

				pos.x =  character->wheelFront;  // roue avant droite
				pos.z = -character->wheelRight;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				h = m_terrain->RetFloorHeight(pos);
				if ( h >  0.5f )  h =  0.5f;
				if ( h < -0.5f )  h = -0.5f;
				pos.x =  front;
				pos.y =  radius-h;
				pos.z = -dist;
				m_object->SetPosition(8, pos);
				if ( type == OBJECT_APOLLO2 )  m_object->SetPosition(12, pos);

				pos.x =  character->wheelFront;  // roue avant gauche
				pos.z =  character->wheelLeft;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				h = m_terrain->RetFloorHeight(pos);
				if ( h >  0.5f )  h =  0.5f;
				if ( h < -0.5f )  h = -0.5f;
				pos.x =  front;
				pos.y =  radius-h;
				pos.z =  dist;
				m_object->SetPosition(9, pos);
				if ( type == OBJECT_APOLLO2 )  m_object->SetPosition(13, pos);
			}
			else
			{
				m_object->SetPosition(6, D3DVECTOR(back,  radius, -dist));
				m_object->SetPosition(7, D3DVECTOR(back,  radius,  dist));
				m_object->SetPosition(8, D3DVECTOR(front, radius, -dist));
				m_object->SetPosition(9, D3DVECTOR(front, radius,  dist));

				if ( type == OBJECT_APOLLO2 )
				{
					m_object->SetPosition(10, D3DVECTOR(back,  radius, -dist));
					m_object->SetPosition(11, D3DVECTOR(back,  radius,  dist));
					m_object->SetPosition(12, D3DVECTOR(front, radius, -dist));
					m_object->SetPosition(13, D3DVECTOR(front, radius,  dist));
				}
			}
		}
	}

	if ( type == OBJECT_MOBILEta ||
		 type == OBJECT_MOBILEtc ||
		 type == OBJECT_MOBILEti ||
		 type == OBJECT_MOBILEts ||
		 type == OBJECT_MOBILErt ||
		 type == OBJECT_MOBILErc ||
		 type == OBJECT_MOBILErr ||
		 type == OBJECT_MOBILErs ||
		 type == OBJECT_MOBILEsa ||
		 type == OBJECT_MOBILEdr )  // chenilles ?
	{
		s = m_physics->RetLinMotionX(MO_MOTSPEED)*0.7f;
		a = m_physics->RetCirMotionY(MO_MOTSPEED)*2.5f;

		m_posTrackLeft  += event.rTime*(s+a);
		m_posTrackRight += event.rTime*(s-a);

		UpdateTrackMapping(m_posTrackLeft, m_posTrackRight, type);

		pos = m_object->RetPosition(0);
		angle = m_object->RetAngle(0);
		if ( pos.x   != m_wheelLastPos.x   ||
			 pos.y   != m_wheelLastPos.y   ||
			 pos.z   != m_wheelLastPos.z   ||
			 angle.x != m_wheelLastAngle.x ||
			 angle.y != m_wheelLastAngle.y ||
			 angle.z != m_wheelLastAngle.z )
		{
			m_wheelLastPos = pos;
			m_wheelLastAngle = angle;

			if ( type == OBJECT_MOBILEta ||
				 type == OBJECT_MOBILEtc ||
				 type == OBJECT_MOBILEti ||
				 type == OBJECT_MOBILEts )
			{
				limit[0] =   8.0f*PI/180.0f;
				limit[1] = -12.0f*PI/180.0f;
			}
			else if ( type == OBJECT_MOBILEsa )
			{
				limit[0] =  15.0f*PI/180.0f;
				limit[1] = -15.0f*PI/180.0f;
			}
			else if ( type == OBJECT_MOBILEdr )
			{
				limit[0] =  10.0f*PI/180.0f;
				limit[1] = -10.0f*PI/180.0f;
			}
			else
			{
				limit[0] =  15.0f*PI/180.0f;
				limit[1] = -10.0f*PI/180.0f;
			}

			if ( Length(pos, m_engine->RetEyePt()) < 50.0f )  // suspension ?
			{
				character = m_object->RetCharacter();
				mat = m_object->RetWorldMatrix(0);

				pos.x =  character->wheelFront;  // roue avant droite
				pos.z = -character->wheelRight;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				a1 = atanf(m_terrain->RetFloorHeight(pos)/character->wheelFront);

				pos.x = -character->wheelBack;  // roue arri�re droite
				pos.z = -character->wheelRight;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				a2 = atanf(m_terrain->RetFloorHeight(pos)/character->wheelBack);

				a = (a2-a1)/2.0f;
				if ( a > limit[0] )  a = limit[0];
				if ( a < limit[1] )  a = limit[1];
				m_object->SetAngleZ(6, a);

				pos.x =  character->wheelFront;  // roue avant gauche
				pos.z =  character->wheelLeft;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				a1 = atanf(m_terrain->RetFloorHeight(pos)/character->wheelFront);

				pos.x = -character->wheelBack;  // roue arri�re gauche
				pos.z =  character->wheelLeft;
				pos.y =  0.0f;
				pos = Transform(*mat, pos);
				a2 = atanf(m_terrain->RetFloorHeight(pos)/character->wheelBack);

				a = (a2-a1)/2.0f;
				if ( a > limit[0] )  a = limit[0];
				if ( a < limit[1] )  a = limit[1];
				m_object->SetAngleZ(7, a);
			}
			else
			{
				m_object->SetAngleZ(6, 0.0f);
				m_object->SetAngleZ(7, 0.0f);
			}
		}
	}

	if ( type == OBJECT_MOBILEwt ||
		 type == OBJECT_MOBILEdr )  // jouet � cl� ?
	{
		pos = m_posKey;
		if ( m_object->RetSelect() &&
			 m_camera->RetType() == CAMERA_ONBOARD )
		{
			pos.y += 10.0f;  // hors du champ de vision !
		}
		m_object->SetPosition(2, pos);

		s  = -Abs(m_physics->RetLinMotionX(MO_MOTSPEED)*0.1f);
		s += -Abs(m_physics->RetCirMotionY(MO_MOTSPEED)*1.5f);
		m_object->SetAngleY(2, m_object->RetAngleY(2)+event.rTime*s);  // tourne la cl�
	}

	if ( type == OBJECT_MOBILEfa ||
		 type == OBJECT_MOBILEfc ||
		 type == OBJECT_MOBILEfi ||
		 type == OBJECT_MOBILEfs ||
		 type == OBJECT_MOBILEft )  // volant ?
	{
		EventFrameFly(event);
	}

	if ( type == OBJECT_MOBILEia ||
		 type == OBJECT_MOBILEic ||
		 type == OBJECT_MOBILEii ||
		 type == OBJECT_MOBILEis )  // pattes ?
	{
		EventFrameInsect(event);
	}

	if ( type == OBJECT_MOBILEwi ||
		 type == OBJECT_MOBILEti ||
		 type == OBJECT_MOBILEfi ||
		 type == OBJECT_MOBILEii )  // canon-insecte ?
	{
		EventFrameCanoni(event);
	}

	return TRUE;
}

// Gestion d'un �v�nement pour un robot volant.

BOOL CMotionVehicle::EventFrameFly(const Event &event)
{
	D3DMATRIX*	mat;
	D3DVECTOR	pos, angle, paw[3];
	float		hope[3], actual, final, h, a;
	int			i;

	pos = m_object->RetPosition(0);
	angle = m_object->RetAngle(0);
	if ( m_bFlyFix                     &&
		 pos.x   == m_wheelLastPos.x   &&
		 pos.y   == m_wheelLastPos.y   &&
		 pos.z   == m_wheelLastPos.z   &&
		 angle.x == m_wheelLastAngle.x &&
		 angle.y == m_wheelLastAngle.y &&
		 angle.z == m_wheelLastAngle.z )  return TRUE;

	m_wheelLastPos = pos;
	m_wheelLastAngle = angle;

	if ( m_physics->RetLand() )  // au sol ?
	{
		mat = m_object->RetWorldMatrix(0);
		paw[0] = Transform(*mat, D3DVECTOR( 4.2f, 0.0f,  0.0f));  // avant
		paw[1] = Transform(*mat, D3DVECTOR(-3.0f, 0.0f, -3.7f));  // arri�re droite
		paw[2] = Transform(*mat, D3DVECTOR(-3.0f, 0.0f,  3.7f));  // arri�re gauche

		for ( i=0 ; i<3 ; i++ )
		{
			h = m_terrain->RetFloorHeight(paw[i]);
			a = -atanf(h*0.5f);
			if ( a >  PI*0.2f )  a =  PI*0.2f;
			if ( a < -PI*0.2f )  a = -PI*0.2f;
			hope[i] = a;
		}
	}
	else	// en vol ?
	{
		hope[0] = 0.0f;  // avant
		hope[1] = 0.0f;  // arri�re droite
		hope[2] = 0.0f;  // arri�re gauche
	}

	m_bFlyFix = TRUE;
	for ( i=0 ; i<3 ; i++ )
	{
		actual = m_object->RetAngleZ(6+i);
		final = Smooth(actual, hope[i], event.rTime*5.0f);
		if ( final != actual )
		{
			m_bFlyFix = FALSE;  // �a a boug�
			m_object->SetAngleZ(6+i, final);
		}
	}

	return TRUE;
}

// Gestion d'un �v�nement pour un insecte � pattes.

BOOL CMotionVehicle::EventFrameInsect(const Event &event)
{
	D3DVECTOR	dir;
	float		s, a, prog, time;
	int			i, st, nd, action;
	BOOL		bStop, bOnBoard;

	static int table[] =
	{
	//	x1,y1,z1,	x2,y2,z2,	x3,y3,z3,	// en l'air :
		60,25,0,	60,0,0,		60,-25,0,	// t0: cuisses 1..4
		-35,0,0,	-35,0,0,	-35,0,0,	// t0: jambes 1..4
		-65,0,0,	-65,0,0,	-65,0,0,	// t0: pieds 1..4
											// au sol devant :
		30,10,0,	30,-15,0,	30,-40,0,	// t1: cuisses 1..4
		-45,0,0,	-45,0,0,	-45,0,0,	// t1: jambes 1..4
		-20,0,0,	-20,0,0,	-20,0,0,	// t1: pieds 1..4
											// au sol derri�re :
		35,40,0,	40,15,0,	40,-10,0,	// t2: cuisses 1..4
		-35,0,0,	-35,0,0,	-35,0,0,	// t2: jambes 1..4
		-50,0,0,	-65,0,0,	-65,0,0,	// t2: pieds 1..4
											// stoppe :
		35,35,0,	40,10,0,	40,-15,0,	// s0: cuisses 1..4
		-35,0,0,	-35,0,0,	-35,0,0,	// s0: jambes 1..4
		-50,0,0,	-65,0,0,	-65,0,0,	// s0: pieds 1..4
	};

	bOnBoard = FALSE;
	if ( m_object->RetSelect() &&
		 m_camera->RetType() == CAMERA_ONBOARD )
	{
		bOnBoard = TRUE;
	}

	s =     m_physics->RetLinMotionX(MO_MOTSPEED)*1.5f;
	a = Abs(m_physics->RetCirMotionY(MO_MOTSPEED)*2.0f);

	if ( s == 0.0f && a != 0.0f )  a *= 1.5f;

	m_armTimeAbs += event.rTime;
	m_armMember += (s+a)*event.rTime*0.15f;

	bStop = ( a == 0.0f && s == 0.0f );  // � l'arr�t ?

	action = 0;  // marche
	if ( s == 0.0f && a == 0.0f )
	{
		action = 3;  // stop
	}

	if ( bStop )
	{
		prog = Mod(m_armTimeAbs, 2.0f)/10.0f;
		a = Mod(m_armMember, 1.0f);
		a = (prog-a)*event.rTime*2.0f;  // vient gentiment � position stop
		m_armMember += a;
	}

	if ( m_object->RetRuin() )  // br�le ou explose ?
	{
		action = 3;
	}

	for ( i=0 ; i<6 ; i++ )  // les 6 pattes
	{
		if ( action != 0 )  // action sp�ciale en cours ?
		{
			st = 3*3*3*action + (i%3)*3;
			nd = st;
			time = event.rTime*5.0f;
		}
		else
		{
			if ( i < 3 )  prog = Mod(m_armMember+(2.0f-(i%3))*0.33f+0.0f, 1.0f);
			else          prog = Mod(m_armMember+(2.0f-(i%3))*0.33f+0.3f, 1.0f);
			if ( prog < 0.33f )  // t0..t1 ?
			{
				prog = prog/0.33f;  // 0..1
				st = 0;  // index start
				nd = 1;  // index end
			}
			else if ( prog < 0.67f )  // t1..t2 ?
			{
				prog = (prog-0.33f)/0.33f;  // 0..1
				st = 1;  // index start
				nd = 2;  // index end
			}
			else	// t2..t0 ?
			{
				prog = (prog-0.67f)/0.33f;  // 0..1
				st = 2;  // index start
				nd = 0;  // index end
			}
			st = 3*3*3*action + st*3*3*3 + (i%3)*3;
			nd = 3*3*3*action + nd*3*3*3 + (i%3)*3;

			// De moins en moins mou ...
			time = event.rTime*20.0f;
		}

		if ( i < 3 )  // patte droite (1..3) ?
		{
			m_object->SetAngleX(6+3*i+0, Smooth(m_object->RetAngleX(6+3*i+0), Prop(table[st+ 0], table[nd+ 0], prog), time));
			m_object->SetAngleY(6+3*i+0, Smooth(m_object->RetAngleY(6+3*i+0), Prop(table[st+ 1], table[nd+ 1], prog), time));
			m_object->SetAngleZ(6+3*i+0, Smooth(m_object->RetAngleZ(6+3*i+0), Prop(table[st+ 2], table[nd+ 2], prog), time));
			m_object->SetAngleX(6+3*i+1, Smooth(m_object->RetAngleX(6+3*i+1), Prop(table[st+ 9], table[nd+ 9], prog), time));
			m_object->SetAngleY(6+3*i+1, Smooth(m_object->RetAngleY(6+3*i+1), Prop(table[st+10], table[nd+10], prog), time));
			m_object->SetAngleZ(6+3*i+1, Smooth(m_object->RetAngleZ(6+3*i+1), Prop(table[st+11], table[nd+11], prog), time));
			m_object->SetAngleX(6+3*i+2, Smooth(m_object->RetAngleX(6+3*i+2), Prop(table[st+18], table[nd+18], prog), time));
			m_object->SetAngleY(6+3*i+2, Smooth(m_object->RetAngleY(6+3*i+2), Prop(table[st+19], table[nd+19], prog), time));
			m_object->SetAngleZ(6+3*i+2, Smooth(m_object->RetAngleZ(6+3*i+2), Prop(table[st+20], table[nd+20], prog), time));
		}
		else	// patte gauche (4..6) ?
		{
			m_object->SetAngleX(6+3*i+0, Smooth(m_object->RetAngleX(6+3*i+0), Prop(-table[st+ 0], -table[nd+ 0], prog), time));
			m_object->SetAngleY(6+3*i+0, Smooth(m_object->RetAngleY(6+3*i+0), Prop(-table[st+ 1], -table[nd+ 1], prog), time));
			m_object->SetAngleZ(6+3*i+0, Smooth(m_object->RetAngleZ(6+3*i+0), Prop( table[st+ 2],  table[nd+ 2], prog), time));
			m_object->SetAngleX(6+3*i+1, Smooth(m_object->RetAngleX(6+3*i+1), Prop(-table[st+ 9], -table[nd+ 9], prog), time));
			m_object->SetAngleY(6+3*i+1, Smooth(m_object->RetAngleY(6+3*i+1), Prop(-table[st+10], -table[nd+10], prog), time));
			m_object->SetAngleZ(6+3*i+1, Smooth(m_object->RetAngleZ(6+3*i+1), Prop( table[st+11],  table[nd+11], prog), time));
			m_object->SetAngleX(6+3*i+2, Smooth(m_object->RetAngleX(6+3*i+2), Prop(-table[st+18], -table[nd+18], prog), time));
			m_object->SetAngleY(6+3*i+2, Smooth(m_object->RetAngleY(6+3*i+2), Prop(-table[st+19], -table[nd+19], prog), time));
			m_object->SetAngleZ(6+3*i+2, Smooth(m_object->RetAngleZ(6+3*i+2), Prop( table[st+20],  table[nd+20], prog), time));
		}
	}

	if ( bStop )
	{
	}
	else
	{
		a = Mod(m_armMember, 1.0f);
		if ( a < 0.5f )  a = -1.0f+4.0f*a;  // -1..1
		else             a =  3.0f-4.0f*a;  // 1..-1
		dir.x = sinf(a)*0.05f;

		s = Mod(m_armMember/2.0f, 1.0f);
		if ( s < 0.5f )  s = -1.0f+4.0f*s;  // -1..1
		else             s =  3.0f-4.0f*s;  // 1..-1
		dir.z = sinf(s)*0.1f;

		dir.y = 0.0f;

		if ( bOnBoard )  dir *= 0.6f;
		SetInclinaison(dir);
	}

	return TRUE;
}

// Gestion d'un �v�nement pour un canon-insecte.

BOOL CMotionVehicle::EventFrameCanoni(const Event &event)
{
	CObject*	power;
	D3DVECTOR	pos, speed;
	FPOINT		dim;
	float		zoom, angle, energy, factor;
	BOOL		bOnBoard = FALSE;

	m_canonTime += event.rTime;

	if ( m_object->RetSelect() &&
		 m_camera->RetType() == CAMERA_ONBOARD )
	{
		bOnBoard = TRUE;
	}

	power = m_object->RetPower();
	if ( power == 0 )
	{
		energy = 0.0f;
	}
	else
	{
		energy = power->RetEnergy();
	}
	if ( energy == 0.0f )  return TRUE;

	factor = 0.5f+energy*0.5f;
	if ( bOnBoard )  factor *= 0.8f;

	zoom = 1.3f+
		   sinf(m_canonTime*PI*0.31f)*0.10f+
		   sinf(m_canonTime*PI*0.52f)*0.08f+
		   sinf(m_canonTime*PI*1.53f)*0.05f;
	zoom *= factor;
	m_object->SetZoomY(2, zoom);

	zoom = 1.0f+
		   sinf(m_canonTime*PI*0.27f)*0.07f+
		   sinf(m_canonTime*PI*0.62f)*0.06f+
		   sinf(m_canonTime*PI*1.73f)*0.03f;
	zoom *= factor;
	m_object->SetZoomZ(2, zoom);

	angle = sinf(m_canonTime*1.0f)*0.10f+
			sinf(m_canonTime*1.3f)*0.15f+
			sinf(m_canonTime*2.7f)*0.05f;
	m_object->SetAngleX(2, angle);

#if 0
	m_lastTimeCanon -= event.rTime;
	if ( m_lastTimeCanon <= 0.0f )
	{
		m_lastTimeCanon = m_engine->ParticuleAdapt(0.5f+Rand()*0.5f);

		pos = m_object->RetPosition(0);
		pos.y += 8.0f;
		speed.y = 7.0f+Rand()*3.0f;
		speed.x = (Rand()-0.5f)*2.0f;
		speed.z = 2.0f+Rand()*2.0f;
		if ( Rand() < 0.5f )  speed.z = -speed.z;
		mat = m_object->RetRotateMatrix(0);
		speed = Transform(*mat, speed);
		dim.x = Rand()*0.1f+0.1f;
		if ( bOnBoard )  dim.x *= 0.4f;
		dim.y = dim.x;
		m_particule->CreateParticule(pos, speed, dim, PARTIORGANIC2, 2.0f, 10.0f);
	}
#endif

	return TRUE;
}


// Met � jour le mapping de la texture des chenilles.

void CMotionVehicle::UpdateTrackMapping(float left, float right, ObjectType type)
{
	D3DMATERIAL7	mat;
	float			limit[4];
	int				rRank, lRank, i;

	ZeroMemory( &mat, sizeof(D3DMATERIAL7) );
	mat.diffuse.r = 1.0f;
	mat.diffuse.g = 1.0f;
	mat.diffuse.b = 1.0f;  // blanc
	mat.ambient.r = 0.5f;
	mat.ambient.g = 0.5f;
	mat.ambient.b = 0.5f;

	rRank = m_object->RetObjectRank(6);
	lRank = m_object->RetObjectRank(7);


	if ( type == OBJECT_MOBILEdr )
	{
		limit[0] = 0.0f;
		limit[1] = 1000000.0f;
		limit[2] = limit[1];
		limit[3] = m_engine->RetLimitLOD(1);

		m_engine->TrackTextureMapping(rRank, mat, D3DSTATEPART1, "drawer.tga", "",
									  limit[0], limit[1], D3DMAPPINGX,
									  right, 1.0f, 8.0f, 192.0f, 256.0f);

		m_engine->TrackTextureMapping(lRank, mat, D3DSTATEPART2, "drawer.tga", "",
									  limit[0], limit[1], D3DMAPPINGX,
									  left, 1.0f, 8.0f, 192.0f, 256.0f);
	}
	else
	{
		limit[0] = 0.0f;
		limit[1] = m_engine->RetLimitLOD(0);
		limit[2] = limit[1];
		limit[3] = m_engine->RetLimitLOD(1);

		for ( i=0 ; i<2 ; i++ )
		{
			m_engine->TrackTextureMapping(rRank, mat, D3DSTATEPART1, "lemt.tga", "",
										  limit[i*2+0], limit[i*2+1], D3DMAPPINGX,
										  right, 1.0f, 8.0f, 192.0f, 256.0f);

			m_engine->TrackTextureMapping(lRank, mat, D3DSTATEPART2, "lemt.tga", "",
										  limit[i*2+0], limit[i*2+1], D3DMAPPINGX,
										  left, 1.0f, 8.0f, 192.0f, 256.0f);
		}
	}

}



// Gestion de l'�tat du crayon du robot dessinateur.

BOOL CMotionVehicle::RetTraceDown()
{
	return m_bTraceDown;
}

void CMotionVehicle::SetTraceDown(BOOL bDown)
{
	m_bTraceDown = bDown;
}

int CMotionVehicle::RetTraceColor()
{
	return m_traceColor;
}

void CMotionVehicle::SetTraceColor(int color)
{
	m_traceColor = color;
}

float CMotionVehicle::RetTraceWidth()
{
	return m_traceWidth;
}

void CMotionVehicle::SetTraceWidth(float width)
{
	m_traceWidth = width;
}


