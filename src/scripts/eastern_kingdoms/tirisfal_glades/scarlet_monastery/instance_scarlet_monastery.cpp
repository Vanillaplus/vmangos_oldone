/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Instance_Scarlet_Monastery
SD%Complete: 50
SDComment:
SDCategory: Scarlet Monastery
EndScriptData */

#include "scriptPCH.h"
#include "scarlet_monastery.h"

enum SM_Misc_Data
{
    NPC_BOSS_LOKSEY     = 25225,
    NPC_BOSS_ABBENDIS   = 25229,
    NPC_BOSS_VISHAS     = 25224,
    NPC_BOSS_HEROD      = 25226,
    NPC_BOSS_MICHAEL    = 25221,
    NPC_BOSS_DOAN       = 25223,
    NPC_BOSS_FAIRBANKS  = 25222,
    NPC_BOSS_MOGRAINE   = 25227,
    NPC_BOSS_WHITEMANE  = 25228,


    GO_LOKSEY_ENTRANCE  = 400028,
    GO_LOKSEY_DOOR      = 400027,
    GO_ABBENDIS_DOOR    = 400026,
    GO_VISHAS_DOOR      = 400025,
    GO_HEROD_DOOR       = 400024,
    GO_HEROD_ENTRANCE   = 400023,
    GO_GARDEN_1         = 400022,
    GO_GARDEN_2         = 400029,
    GO_DOAN_DOOR        = 400020,
    GO_HOLY_DOOR        = 400021,
    GO_WHITEMANE_DOOR   = 400030,
    GO_SECRET_DOOR_1    = 400031,
    GO_SECRET_DOOR_2    = 400032,
    GO_SECRET_TORCH     = 97701,



    // Ashbringer Event misc
    NPC_SCARLET_MYRIDON = 4295,
    NPC_SCARLET_DEFENDER = 4298,
    NPC_SCARLET_CENTURION = 4301,
    NPC_SCARLET_SORCERER = 4294,
    NPC_SCARLET_WIZARD = 4300,
    NPC_SCARLET_ABBOT = 4303,
    NPC_SCARLET_MONK = 4540,
    NPC_SCARLET_CHAMPION = 4302,
    NPC_SCARLET_CHAPLAIN = 4299,
    NPC_FAIRBANKS = 4542,
    NPC_COMMANDER_MOGRAINE = 3976,
    NPC_HIGHLORD_MOGRAINE = 16062,

    SAY_ASHBRINGER    = -1900216,
    SAY_KNEEL_BEFORE  = -1900217,
    SAY_MY_LORD       = -1900218,
    SAY_TAKE_ME       = -1900219,
    SAY_CLEANSE_WORLD = -1900227,
    SAY_AND_SO_IT_BEG = -1900228,
    GO_CHAPPEL_DOOR   = 104591,

    SAY_COMMANDER1  = -1900220,
    SAY_COMMANDER2  = -1900221,
    SAY_COMMANDER3  = -1900222,
    SAY_ASHBRINGER1 = -1900223,
    SAY_ASHBRINGER2 = -1900224,
    SAY_ASHBRINGER3 = -1900225,

    YELL_COMMANDER  = -1900226,
    SPELL_FORGIVENESS     = 28697,
    SPELL_MOGRAINE_COMETH = 28688,
};

enum eEvents
{
    EVENT_KNEEL = 1,
    EVENT_TALK1,
    EVENT_SUMMON,
    EVENT_TALK2,
    EVENT_STAND,
    EVENT_TALK3,
    EVENT_TALK4,
    EVENT_ROAR,
    EVENT_TALK5,
    EVENT_SPELL,
    EVENT_FORGIVEN,
    EVENT_DESPAWN
};

struct instance_scarlet_monastery : ScriptedInstance
{
    explicit instance_scarlet_monastery(Map* pMap) : 
        ScriptedInstance(pMap),
        m_ashbringerActive(false)
    {
        instance_scarlet_monastery::Initialize();
    }

    uint32 m_auiEncounter[INSTANCE_SM_MAX_ENCOUNTER];
    uint64 m_auiData[MAX_DATAS];
    std::string strInstData;

    uint64 m_uiMograineGUID;
    uint64 m_uiWhitemaneGUID;
    
    bool m_ashbringerActive;
    uint32 m_ashbringerCheckTimer;
    std::set<ObjectGuid> m_ashbringerReactedNpcs;
    uint32 m_ashbringerSayTimer;
    EventMap m_events;

    void Initialize() override
    {
        memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        memset(&m_auiData, 0, sizeof(m_auiData));

        m_uiMograineGUID = 0;
        m_uiWhitemaneGUID = 0;

        m_ashbringerCheckTimer = 5000;
        m_ashbringerReactedNpcs.clear();
        m_ashbringerSayTimer = 0;
        m_events.Reset();
    }

    void OnCreatureCreate(Creature* pCreature) override
    {
        switch (pCreature->GetEntry())
        {
            case NPC_BOSS_MOGRAINE:
                m_uiMograineGUID = pCreature->GetGUID();
                break;
            case NPC_BOSS_WHITEMANE:
                m_uiWhitemaneGUID = pCreature->GetGUID();
                break;
        }
    }

    void OnObjectCreate(GameObject* pGo) override
    {
        switch (pGo->GetEntry())
        {
            case GO_LOKSEY_ENTRANCE:
                m_auiData[DATA_DOOR_LOKSEY_ENTRANCE] = pGo->GetObjectGuid();
                break;
            case GO_LOKSEY_DOOR:
                m_auiData[DATA_DOOR_LOKSEY_DOOR] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_LOKSEY] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_ABBENDIS_DOOR:
                m_auiData[DATA_DOOR_ABBENDIS_DOOR] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_ABBENDIS] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_VISHAS_DOOR:
                m_auiData[DATA_DOOR_VISHAS_DOOR] = pGo->GetObjectGuid();
                break;
            case GO_HEROD_ENTRANCE:
                m_auiData[DATA_DOOR_HEROD_ENTRANCE] = pGo->GetObjectGuid();
                break;
            case GO_HEROD_DOOR:
                m_auiData[DATA_DOOR_HEROD_DOOR] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_HEROD] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;
            case GO_GARDEN_1:
                m_auiData[DATA_DOOR_MICHAEL_DOOR_1] = pGo->GetObjectGuid();
                break;
            case GO_GARDEN_2:
                m_auiData[DATA_DOOR_MICHAEL_DOOR_2] = pGo->GetObjectGuid();
                break;
            case GO_DOAN_DOOR:
                m_auiData[DATA_DOOR_DOAN_DOOR] = pGo->GetObjectGuid();
                break;
            case GO_HOLY_DOOR:
                m_auiData[DATA_DOOR_MOGRAINE_ENTRANCE] = pGo->GetObjectGuid();
                break;
            case GO_SECRET_TORCH:
                m_auiData[DATA_SECRET_TORCH] = pGo->GetObjectGuid();
                break;
            case GO_SECRET_DOOR_1:
                m_auiData[DATA_DOOR_FAIRBANKS_DOOR_1] = pGo->GetObjectGuid();
                break;
            case GO_SECRET_DOOR_2:
                m_auiData[DATA_DOOR_FAIRBANKS_DOOR_2] = pGo->GetObjectGuid();
                break;
            case GO_WHITEMANE_DOOR:
                m_auiData[DATA_DOOR_MOGRAINE_DOOR] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_MOGRAINE_AND_WHITE_EVENT] == DONE)
                    pGo->SetGoState(GO_STATE_ACTIVE);
                break;

        }
    }

    void SetData(uint32 uiType, uint32 uiData) override
    {
        switch (uiType)
        {
            case TYPE_LOKSEY:
            {
                m_auiEncounter[TYPE_LOKSEY] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_LOKSEY_ENTRANCE]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_LOKSEY_ENTRANCE]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_LOKSEY_ENTRANCE]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_LOKSEY_ENTRANCE]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_LOKSEY_ENTRANCE]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_LOKSEY_ENTRANCE]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_LOKSEY_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_LOKSEY_DOOR]);
                    }
                }
                break;
            }
            case TYPE_ABBENDIS:
            {
                m_auiEncounter[TYPE_ABBENDIS] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_LOKSEY_DOOR]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_LOKSEY_DOOR]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_LOKSEY_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_LOKSEY_DOOR]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_LOKSEY_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_LOKSEY_DOOR]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_ABBENDIS_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_ABBENDIS_DOOR]);
                    }
                }
                break;
            }
            case TYPE_VISHAS:
            {
                m_auiEncounter[TYPE_VISHAS] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_VISHAS_DOOR]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_VISHAS_DOOR]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_VISHAS_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_VISHAS_DOOR]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_VISHAS_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_VISHAS_DOOR]);
                    }
                }
                break;
            }
            case TYPE_HEROD:
            {
                m_auiEncounter[TYPE_HEROD] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_HEROD_ENTRANCE]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_HEROD_ENTRANCE]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_HEROD_ENTRANCE]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_HEROD_ENTRANCE]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_HEROD_ENTRANCE]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_HEROD_ENTRANCE]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_HEROD_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_HEROD_DOOR]);
                    }
                }
                break;
            }
            case TYPE_MICHAEL:
            {
                m_auiEncounter[TYPE_MICHAEL] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MICHAEL_DOOR_1]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MICHAEL_DOOR_1]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MICHAEL_DOOR_2]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MICHAEL_DOOR_2]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MICHAEL_DOOR_1]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MICHAEL_DOOR_1]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MICHAEL_DOOR_2]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MICHAEL_DOOR_2]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MICHAEL_DOOR_1]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MICHAEL_DOOR_1]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MICHAEL_DOOR_2]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MICHAEL_DOOR_2]);
                    }
                }
                break;
            }
            case TYPE_DOAN:
            {
                m_auiEncounter[TYPE_DOAN] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_DOAN_DOOR]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_DOAN_DOOR]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_DOAN_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_DOAN_DOOR]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_DOAN_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_DOAN_DOOR]);
                    }
                }
                break; 
            }
            case TYPE_FAIRBANKS:
            {
                m_auiEncounter[TYPE_FAIRBANKS] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_1]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_1]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_2]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_2]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_1]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_1]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_2]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_2]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_1]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_1]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_2]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_FAIRBANKS_DOOR_2]);
                    }
                }
                break;
            }
            case TYPE_MOGRAINE_AND_WHITE_EVENT:
            {
                m_auiEncounter[TYPE_MOGRAINE_AND_WHITE_EVENT] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MOGRAINE_DOOR]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MOGRAINE_DOOR]);
                    }
                }
                else if (uiData == FAIL)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MOGRAINE_ENTRANCE]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MOGRAINE_ENTRANCE]);
                    }
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MOGRAINE_DOOR]))
                    {
                        if (pGo->GetGoState() == GO_STATE_ACTIVE) // Open
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MOGRAINE_DOOR]);
                    }
                }
                else if (uiData == DONE)
                {
                    if (GameObject* pGo = instance->GetGameObject(m_auiData[DATA_DOOR_MOGRAINE_ENTRANCE]))
                    {
                        if (pGo->GetGoState() != GO_STATE_ACTIVE) // Close
                            DoUseDoorOrButton(m_auiData[DATA_DOOR_MOGRAINE_ENTRANCE]);
                    }
                }
                break;
            }
        }


        if (uiData == DONE)
        {
            std::ostringstream saveStream;
            for (uint8 i = 0; i < INSTANCE_SM_MAX_ENCOUNTER; ++i)
                saveStream << m_auiEncounter[i] << " ";

            strInstData = saveStream.str();
            SaveToDB();
        }
    }

    uint32 GetData(uint32 uiType) override
    {
        if (uiType < INSTANCE_SM_MAX_ENCOUNTER)
            return m_auiEncounter[uiType];
        return 0;
    }

    uint64 GetData64(uint32 uiData) override
    {
        if (uiData < MAX_DATAS)
            return m_auiData[uiData];
        return 0;
    }

    const char* Save() override
    {
        return strInstData.c_str();
    }

    void Load(const char* chrIn) override
    {
        if (!chrIn)
            return;

        std::istringstream loadStream(chrIn);
        for (uint8 i = 0; i < INSTANCE_SM_MAX_ENCOUNTER; ++i)
            loadStream >> m_auiEncounter[i];

        for (uint8 i = 0; i < INSTANCE_SM_MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS)
                m_auiEncounter[i] = NOT_STARTED;
    }

    uint32 GetRandomScarletText()
    {
        switch (urand(0, 6))
        {
            case 0:
                return 12378;
            case 1:
                return 12379;
            case 2:
                return 12380;
            case 3:
                return 12381;
            case 4:
                return 12382;
            case 5:
                return 12383;
        }
        return 12384;
    }

    void OnCreatureSpellHit(Unit* pCaster, Creature* receiver, const SpellEntry* spell) override
    {
        if (!m_ashbringerActive || !pCaster || !receiver || !spell)
            return;
        if (!pCaster->IsPlayer() || spell->Id != 28441) // AB Effect 000
            return;
        if (receiver->isDead())
            return;

        switch (receiver->GetEntry())
        {
        case NPC_SCARLET_MYRIDON:
        case NPC_SCARLET_DEFENDER:
        case NPC_SCARLET_CENTURION:
        case NPC_SCARLET_SORCERER:
        case NPC_SCARLET_WIZARD:
        case NPC_SCARLET_ABBOT:
        case NPC_SCARLET_MONK:
        case NPC_SCARLET_CHAMPION:
        case NPC_SCARLET_CHAPLAIN:
        {
            auto it = m_ashbringerReactedNpcs.find(receiver->GetObjectGuid());
            if (it == m_ashbringerReactedNpcs.end())
                m_ashbringerReactedNpcs.insert(receiver->GetObjectGuid());
            else
                return;
            receiver->StopMoving(true);
            receiver->SetFacingToObject(pCaster);
            receiver->SetStandState(UNIT_STAND_STATE_KNEEL);
            receiver->addUnitState(UNIT_STAT_ROOT);
            if (!m_ashbringerSayTimer && urand(0,1))
            {
                m_ashbringerSayTimer = 2000;
                DoScriptText(GetRandomScarletText(), receiver);
            }
            break;
        }
        case NPC_COMMANDER_MOGRAINE:
        {
            auto it = m_ashbringerReactedNpcs.find(receiver->GetObjectGuid());
            if (it == m_ashbringerReactedNpcs.end())
                m_ashbringerReactedNpcs.insert(receiver->GetObjectGuid());
            else
                return;
            receiver->SetFacingToObject(pCaster);
            m_events.ScheduleEvent(EVENT_KNEEL, Seconds(1));
            break;
        }
        default:
            return;
        }
    }

    void Update(uint32 diff) override
    {
        // not bothering to check if player has unequipped weapon, dont know if its a thing
        if (m_ashbringerActive)
        {
            // https://www.youtube.com/watch?v=nT6yE7vuCJc
            // todo: emotes are a bit off, should be more of them too
            m_events.Update(diff);
            while (uint32 eventId = m_events.ExecuteEvent())
            {
                Creature* pMograine = GetCreature(m_uiMograineGUID);
                if (!pMograine)
                    continue;
                Creature* pHighlord = GetClosestCreatureWithEntry(pMograine, NPC_HIGHLORD_MOGRAINE, 1000.0f);
                switch (eventId)
                {
                    case EVENT_KNEEL:
                        pMograine->SetSheath(SHEATH_STATE_UNARMED);
                        pMograine->SetStandState(UNIT_STAND_STATE_KNEEL);
                        m_events.ScheduleEvent(EVENT_TALK1, Seconds(2));
                        break;
                    case EVENT_TALK1:
                        DoScriptText(SAY_COMMANDER1, pMograine);
                        m_events.ScheduleEvent(EVENT_SUMMON, Seconds(10));
                        break;
                    case EVENT_SUMMON:
                        if (Creature* pC = pMograine->SummonCreature(NPC_HIGHLORD_MOGRAINE, 1065.130737f, 1399.350586f, 30.763723f, 6.282961f, TEMPSUMMON_TIMED_DESPAWN, 400000))
                        {
                            pC->SetFactionTemporary(35, TEMPFACTION_RESTORE_RESPAWN);
                            pC->SetObjectScale(2.0f);
                            pC->CastSpell(pC, SPELL_MOGRAINE_COMETH, true);
                            pC->GetMotionMaster()->MovePoint(0, 1148.0f, 1398.71f, 32.0f, 0, 2.7f);
                        }
                        m_events.ScheduleEvent(EVENT_TALK2, Seconds(30));
                        break;
                    case EVENT_TALK2:
                        if (pHighlord)
                        {
                            pHighlord->HandleEmote(EMOTE_ONESHOT_POINT);
                            DoScriptText(SAY_ASHBRINGER1, pHighlord);
                            pMograine->SetFacingToObject(pHighlord);
                        }
                        m_events.ScheduleEvent(EVENT_STAND, Seconds(4));
                        break;
                    case EVENT_STAND:
                        pMograine->SetStandState(UNIT_STAND_STATE_STAND);
                        m_events.ScheduleEvent(EVENT_TALK3, Seconds(2));
                        break;
                    case EVENT_TALK3:
                        DoScriptText(SAY_COMMANDER2, pMograine);
                        m_events.ScheduleEvent(EVENT_TALK4, Seconds(4));
                        break;
                    case EVENT_TALK4:
                        if(pHighlord)
                            DoScriptText(SAY_ASHBRINGER2, pHighlord);
                        m_events.ScheduleEvent(EVENT_ROAR, Seconds(11));
                        break;
                    case EVENT_ROAR:
                        if (pHighlord)
                            pHighlord->HandleEmote(EMOTE_ONESHOT_BATTLEROAR);
                        m_events.ScheduleEvent(EVENT_TALK5, Seconds(4));
                        break;
                    case EVENT_TALK5:
                        pMograine->SetSheath(SHEATH_STATE_UNARMED);
                        pMograine->HandleEmote(EMOTE_ONESHOT_BEG);
                        DoScriptText(SAY_COMMANDER3, pMograine);
                        m_events.ScheduleEvent(EVENT_SPELL, Seconds(3));
                        break;
                    case EVENT_SPELL:
                        if (pHighlord)
                            pHighlord->CastSpell(pMograine, SPELL_FORGIVENESS, false);
                        m_events.ScheduleEvent(EVENT_FORGIVEN, Seconds(4));
                        break;
                    case EVENT_FORGIVEN:
                        if (pHighlord)
                            DoScriptText(SAY_ASHBRINGER3, pHighlord);
                        m_events.ScheduleEvent(EVENT_DESPAWN, Seconds(5));
                        break;
                    case EVENT_DESPAWN:
                        if (pHighlord)
                        {
                            ((TemporarySummon*)pHighlord)->UnSummon();
                        }
                        break;
                }
            }
            m_ashbringerSayTimer -= std::min(m_ashbringerSayTimer, diff);
            return;
        }


        if (m_ashbringerCheckTimer < diff)
        {
            m_ashbringerCheckTimer = 5000;
        }
        else
        {
            m_ashbringerCheckTimer -= diff;
            return;
        }

        Map::PlayerList const& lPlayers = instance->GetPlayers();
        if (lPlayers.isEmpty())
            return;
        bool anyAshbringerEquipped = false;
        for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
        {
            if (Player* pPlayer = itr->getSource())
            {
                Item* item = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
                if (!item)
                    continue;
                if (item->GetEntry() == 22691 && !m_ashbringerActive) // corrupted ashbringer
                {
                    anyAshbringerEquipped = true;
                    m_ashbringerActive = true;
                    std::list<Creature*> ScarletList;
                    GetCreatureListWithEntryInGrid(ScarletList, pPlayer,
                    { 
                        NPC_SCARLET_MYRIDON,
                        NPC_SCARLET_DEFENDER,
                        NPC_SCARLET_CENTURION,
                        NPC_SCARLET_SORCERER,
                        NPC_SCARLET_WIZARD,
                        NPC_SCARLET_ABBOT,
                        NPC_SCARLET_MONK, 
                        NPC_SCARLET_CHAMPION,
                        NPC_SCARLET_CHAPLAIN,
                        NPC_FAIRBANKS,
                        NPC_COMMANDER_MOGRAINE
                    }, 2000.0f);

                    for (Creature* pCreature : ScarletList)
                    {
                        pCreature->setFaction(35);
                        if (pCreature->GetEntry() == NPC_COMMANDER_MOGRAINE)
                            pCreature->SetCharmerGuid(pPlayer->GetObjectGuid());
                    }

                    // Door should be open when event is activated
                    if (GameObject* pDoor = GetClosestGameObjectWithEntry(pPlayer, GO_CHAPPEL_DOOR, 2000.0f))
                        DoOpenDoor(pDoor->GetGUID());


                    // bow down, kneel before the ashbringer... scripttext yell
                    DoOrSimulateScriptTextForThisInstance(YELL_COMMANDER, NPC_COMMANDER_MOGRAINE);
                }
            }
        }
    }
};

InstanceData* GetInstanceData_instance_scarlet_monastery(Map* pMap)
{
    return new instance_scarlet_monastery(pMap);
}

void AddSC_instance_scarlet_monastery()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_scarlet_monastery";
    newscript->GetInstanceData = &GetInstanceData_instance_scarlet_monastery;
    newscript->RegisterSelf();
}
