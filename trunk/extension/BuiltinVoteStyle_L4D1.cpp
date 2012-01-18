/**
 * vim: set ts=4 :
 * =============================================================================
 * Builtin Votes
 * Copyright (C) 2011 Ross Bemrose (Powerlord).  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "BuiltinVoteStyle_L4D1.h"

/* This will error out if you try to compile for lower source engines.  Since this class should only be included
 * for recent engines, I don't see that as a problem
 */ 
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);

L4D1BuiltinVoteStyle g_L4D1BuiltinVoteStyle;

/*
void L4D1BuiltinVoteStyle::SendDisplay(int clients[], unsigned int num_clients, IBaseBuiltinVote *vote)
{
	vote->Display(clients, num_clients);
}
*/

IBaseBuiltinVote *L4D1BuiltinVoteStyle::CreateVote(IBuiltinVoteHandler *handler, BuiltinVoteType type, IdentityToken_t *pOwner)
{
	if (CheckVoteType(type))
	{
		return new CL4D1BuiltinVote(handler, this, type, pOwner);
	}
	else
	{
		return NULL;
	}
}

const char *L4D1BuiltinVoteStyle::GetStyleName()
{
	return "l4d1";
}

unsigned int L4D1BuiltinVoteStyle::GetApproxMemUsage()
{
	return sizeof(L4D1BuiltinVoteStyle) + (sizeof(CBaseBuiltinVotePlayer) * 257);
}

CL4D1BuiltinVote::CL4D1BuiltinVote(IBuiltinVoteHandler *pHandler, IBuiltinVoteStyle *pStyle, BuiltinVoteType type, IdentityToken_t *pOwner) :
CL4DBaseBuiltinVote(pHandler, pStyle, type, pOwner)
{
}

CL4D1BuiltinVote::~CL4D1BuiltinVote()
{
	if (!m_bResultDisplayed)
	{
		SH_REMOVE_HOOK(IServerGameClients, ClientCommand, servergameclients, SH_MEMBER(this, &CL4D1BuiltinVote::OnClientCommand), false);
	}
}

bool CL4D1BuiltinVote::Display(int clients[], unsigned int num_clients)
{
	if (m_bCancelling)
	{
		return false;
	}

	const char *translation = GetStartTranslation();

	IGameEvent *startEvent = events->CreateEvent("vote_started");
	startEvent->SetInt("team", m_team);
	startEvent->SetInt("initiator", m_initiator);
	startEvent->SetString("issue", translation);
	startEvent->SetString("param1", GetArgument());

	events->FireEvent(startEvent);

	SH_ADD_HOOK(IServerGameClients, ClientCommand, servergameclients, SH_MEMBER(this, &CL4D1BuiltinVote::OnClientCommand), false);

	return true;
}

unsigned int CL4D1BuiltinVote::GetApproxMemUsage()
{
	return sizeof(CL4D1BuiltinVote) + GetBaseMemUsage();
}

void CL4D1BuiltinVote::Cancel_Finally()
{
	g_L4D1BuiltinVoteStyle.CancelVote(this);
}

void CL4D1BuiltinVote::VoteEnded()
{
	IGameEvent *endEvent = events->CreateEvent("vote_ended");
	events->FireEvent(endEvent);
}

void CL4D1BuiltinVote::DisplayVotePass(const char *winner)
{
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, servergameclients, SH_MEMBER(this, &CL4D1BuiltinVote::OnClientCommand), false);

	VoteEnded();

	const char *translation = GetPassTranslation();

	IGameEvent *passEvent = events->CreateEvent("vote_passed");
	passEvent->SetString("details", translation);
	passEvent->SetString("param1", winner);
	passEvent->SetInt("team", m_team);
	events->FireEvent(passEvent);
}

void CL4D1BuiltinVote::DisplayVoteFail(BuiltinVoteFailReason reason)
{
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, servergameclients, SH_MEMBER(this, &CL4D1BuiltinVote::OnClientCommand), false);

	VoteEnded();
	
	IGameEvent *failEvent = events->CreateEvent("vote_failed");
	failEvent->SetInt("team", m_team);
	events->FireEvent(failEvent);

}


void CL4D1BuiltinVote::ClientSelectedItem(int client, unsigned int item)
{
	//CBaseBuiltinVote::ClientSelectedItem(client, item);

	if (item <= BUILTINVOTES_VOTE_YES)
	{
		IGameEvent *castEvent;
		switch (item)
		{
		case BUILTINVOTES_VOTE_NO:
			castEvent = events->CreateEvent("vote_cast_no");
			break;

		case BUILTINVOTES_VOTE_YES:
			castEvent = events->CreateEvent("vote_cast_yes");
			break;

		}

		castEvent->SetInt("team", m_team);
		castEvent->SetInt("entityid", m_initiator);
		events->FireEvent(castEvent);

	}
}

