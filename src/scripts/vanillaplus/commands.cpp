#include "scriptPCH.h"
#include "BattleGroundMgr.h"
#include "Chat.h"
#include "World.h"
#include "Player.h"

bool ChatHandler::HandleReloadShopCommand(char* args)
{
    sObjectMgr.LoadShop();
    SendSysMessage("DB table `shop_categories` reloaded.");
    SendSysMessage("DB table `shop_entries` reloaded.");
    return true;
}

bool ChatHandler::HandleSaleCommand(char* args)
{
    if (!*args)
    {
        SendSysMessage("Syntax: .sale on / off");
        SetSentErrorMessage(true);
        return false;
    }

    bool value;

    if (!ExtractOnOff(&args, value))
    {
        SetSentErrorMessage(true);
        return false;
    }

    if (value)
    {
        WorldDatabase.PExecute("UPDATE shop_items SET price = price / 2;");
        SendSysMessage("Shop sale is ON.");
    }
    else
    {
        WorldDatabase.PExecute("UPDATE shop_items SET price = price * 2;");
        SendSysMessage("Shop sale is OFF.");
    }
    SendSysMessage("Use this command to update prices: |cff0ccaec.reload shop|r");
    return true;
}

bool ChatHandler::HandleReloadCustomPetEntries(char *args)
{
    sObjectMgr.LoadCustomPetCreatureEntries();
    return true;
}
   
#define CHARACTER_MANAGEMENT_SKIN     60001
#define CHARACTER_MANAGEMENT_RACE     60002
#define CHARACTER_MANAGEMENT_FACTION  61434

class DemorphAfterTime : public BasicEvent
{
public:
    explicit DemorphAfterTime(uint64 player_guid) : BasicEvent(), player_guid(player_guid) {}

    bool Execute(uint64 e_time, uint32 p_time) override
    {
        Player* player = ObjectAccessor::FindPlayer(player_guid);
        if (player)
        {
            player->DeMorph();
        }
        return false;
    }

private:
    uint64 player_guid;
};

bool ChatHandler::HandleSkinCommand(char* args)
{
    if (!args || !*args)
    {
        PSendSysMessage("You must specify the name of the character you want to look like.");
        SetSentErrorMessage(true);
        return false;
    }

    if (Player* target = m_session->GetPlayer())
    {
        if (!target->HasItemCount(CHARACTER_MANAGEMENT_SKIN))
        {
            PSendSysMessage("You must purchase [Character: Look Change] first.");
            SetSentErrorMessage(true);
            return false;
        }

        uint8 curr_race = target->getRace();
        std::string plName = args;
        CharacterDatabase.escape_string(plName);

        std::wstring wPlName;

        if (!Utf8toWStr(plName, wPlName))
            return false;

        wstrToLower(wPlName);

        if (Utf8FitTo(target->GetName(), wPlName))
        {
            PSendSysMessage("You must specify a name of a character different than yourself!");
            SetSentErrorMessage(true);
            return false;
        }

        if (target->isInCombat() || target->InBattleGround() || target->HasSpellCooldown(20939) || (target->getDeathState() == CORPSE) || target->IsBeingTeleported())
        {
            PSendSysMessage("You can not change your appearance yet.");
            SetSentErrorMessage(true);
            return false;
        }

        auto result = std::unique_ptr<QueryResult>{ CharacterDatabase.PQuery("SELECT race, playerBytes, playerBytes2 & 0xFF, gender FROM characters WHERE name='%s'", plName.c_str()) };

        if (!result)
        {
            PSendSysMessage("You must specify the name of the character you want to look like.");
            SetSentErrorMessage(true);
            return false;
        }

        Field* fields = result->Fetch();
        uint8 race = fields[0].GetUInt8();
        uint32 bytes = fields[1].GetUInt32();
        uint32 bytes2 = fields[2].GetUInt32();
        uint8 gender = fields[3].GetUInt8();

        if (race != curr_race)
        {
            PSendSysMessage("It should be a character of the same race.");
            SetSentErrorMessage(true);
            return false;
        }
        else
        {
            bytes2 |= (target->GetUInt32Value(PLAYER_BYTES_2) & 0xFFFFFF00);
            target->SetUInt32Value(PLAYER_BYTES, bytes);
            target->SetUInt32Value(PLAYER_BYTES_2, bytes2);
            target->SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);
            target->DestroyItemCount(CHARACTER_MANAGEMENT_SKIN, 1, true, false, true);
            target->SaveInventoryAndGoldToDB();
            target->SetDisplayId(15435); // Invisible
            target->m_Events.AddEvent(new DemorphAfterTime(target->GetGUID()), target->m_Events.CalculateTime(250));
            return true;
        }
    }

    return false;
}

bool ChatHandler::HandleFactionCommand(char* args)
{
    if (!args || !*args)
    {
        PSendSysMessage("You must specify the name of the character you want to look like.");
        SetSentErrorMessage(true);
        return false;
    }

    if (Player* target = m_session->GetPlayer())
    {
        if (!target->HasItemCount(CHARACTER_MANAGEMENT_FACTION))
        {
            PSendSysMessage("You must purchase [Character: Faction Change] first.");
            SetSentErrorMessage(true);
            return false;
        }
        std::string plName = args;
        CharacterDatabase.escape_string(plName);

        std::wstring wPlName;

        if (!Utf8toWStr(plName, wPlName))
            return false;

        wstrToLower(wPlName);

        if (Utf8FitTo(target->GetName(), wPlName))
        {
            PSendSysMessage("You must specify a name of a character different than yourself!");
            SetSentErrorMessage(true);
            return false;
        }

        if (target->isInCombat() || target->InBattleGround() || target->HasSpellCooldown(20939) || (target->getDeathState() == CORPSE) || target->IsBeingTeleported())
        {
            PSendSysMessage("You can not change your faction yet.");
            SetSentErrorMessage(true);
            return false;
        }

        auto result = std::unique_ptr<QueryResult>{ CharacterDatabase.PQuery("SELECT race, class, playerBytes, playerBytes2 & 0xFF, gender FROM characters WHERE name='%s'", plName.c_str()) };
        if (!result)
        {
            PSendSysMessage("We don't know this guy! You must specify the name of the character you want to look like.");
            SetSentErrorMessage(true);
            return false;
        }

        Field* fields = result->Fetch();
        uint32 new_race = fields[0].GetUInt32();
        uint8 class_ = fields[1].GetUInt8();
        uint32 bytes = fields[2].GetUInt32();
        uint32 bytes2 = fields[3].GetUInt32();
        uint8 gender = fields[4].GetUInt8();

        if (target->getClass() != class_)
        {
            PSendSysMessage("Wrong class. It should be a character of the same class.");
            SetSentErrorMessage(true);
            return false;
        }

        bytes2 |= (target->GetUInt32Value(PLAYER_BYTES_2) & 0xFFFFFF00);

        target->SetUInt32Value(PLAYER_BYTES, bytes);
        target->SetUInt32Value(PLAYER_BYTES_2, bytes2);
        target->SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);

        target->DestroyItemCount(CHARACTER_MANAGEMENT_FACTION, 1, true, false, true);
        target->SaveInventoryAndGoldToDB();

        target->ChangeRace(new_race, gender, bytes, bytes2);
        return true;
    }
    return false;
}

bool ChatHandler::HandleRaceCommand(char* args)
{
    if (!args || !*args)
    {
        PSendSysMessage("You must specify the name of the character you want to look like.");
        SetSentErrorMessage(true);
        return false;
    }

    if (Player* target = m_session->GetPlayer())
    {
        if (!target->HasItemCount(CHARACTER_MANAGEMENT_RACE))
        {
            PSendSysMessage("You must purchase [Character: Race Change] first!");
            SetSentErrorMessage(true);
            return false;
        }

        uint8 curr_race = target->getRace();
        uint8 curr_class = target->getClass();

        const char* curr_class_name = GetClassName(curr_class, GetSessionDbcLocale());

        std::string plName = args;
        CharacterDatabase.escape_string(plName);

        std::wstring wPlName;

        if (!Utf8toWStr(plName, wPlName))
            return false;

        wstrToLower(wPlName);


        if (Utf8FitTo(target->GetName(), wPlName))
        {
            PSendSysMessage("You must specify a name of a character different than yourself!");
            SetSentErrorMessage(true);
            return false;
        }

        if (target->isInCombat() || target->InBattleGround() || target->HasSpellCooldown(20939) || (target->getDeathState() == CORPSE) || target->IsBeingTeleported())
        {
            PSendSysMessage("You can not change your race yet.");
            SetSentErrorMessage(true);
            return false;
        }

        auto result = std::unique_ptr<QueryResult>{ CharacterDatabase.PQuery("SELECT race, class, playerBytes, playerBytes2 & 0xFF, gender FROM characters WHERE name='%s'", plName.c_str()) };
        if (!result)
        {
            PSendSysMessage("We don't know this guy! You must specify the name of the character you want to look like.");
            SetSentErrorMessage(true);
            return false;
        }

        Field* fields = result->Fetch();
        uint32 new_race = fields[0].GetUInt32();
        uint8 class_ = fields[1].GetUInt8();
        uint32 bytes = fields[2].GetUInt32();
        uint32 bytes2 = fields[3].GetUInt32();
        uint8 gender = fields[4].GetUInt8();

        ChrRacesEntry const* curr_race_entry = sChrRacesStore.LookupEntry(curr_race);
        ChrRacesEntry const* new_race_entry = sChrRacesStore.LookupEntry(new_race);

        if (class_ != curr_class || curr_race_entry->TeamID != new_race_entry->TeamID)
        {
            PSendSysMessage("This character should be a part of your faction, and must be a %s.", curr_class_name);
            SetSentErrorMessage(true);
            return false;
        }
        else
        {
            bytes2 |= (target->GetUInt32Value(PLAYER_BYTES_2) & 0xFFFFFF00);

            target->SetUInt32Value(PLAYER_BYTES, bytes);
            target->SetUInt32Value(PLAYER_BYTES_2, bytes2);
            target->SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);

            target->DestroyItemCount(CHARACTER_MANAGEMENT_RACE, 1, true, false, true);
            target->SaveInventoryAndGoldToDB();

            target->ChangeRace(new_race, gender, bytes, bytes2);

            return true;
        }
    }

    return false;
}

bool RegisterPlayerToBG(WorldSession * sess, BattleGroundTypeId bgid)
{
    Player* pPlayer = sess->GetPlayer();
    if (!pPlayer->GetBGAccessByLevel(bgid))
        return false;
    pPlayer->SetBattleGroundEntryPoint(pPlayer->GetMapId(), pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ(), pPlayer->GetOrientation());
    sess->SendBattlegGroundList(pPlayer->GetObjectGuid(), bgid);
    return true;
}

bool ChatHandler::HandleArenaCommand(char* args)
{
    Player* player = m_session->GetPlayer();

    if (player->InBattleGround() || player->GetInstanceId())
    {
        SendSysMessage("You cannot use this command here.");
        return false;
    }

    if (player->getLevel() < 60)
    {
        SendSysMessage("Your level is not high enough.");
        return false;
    }

    if (!player->EnoughPvPRankToArena())
    {
        SendSysMessage("Your PvP Rank is not high enough.");
        return false;
    }

    if (!args)
    {
        SendSysMessage("Syntax: .arena 2 | .arena 3");
        return true;
    }

    if (strcmp(args, "2") == 0)
    {
        return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_ARENA_BRA_2));
    }
    else if (strcmp(args, "3") == 0)
    {
        return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_ARENA_BRA_3));
    }
    else
    {
        SendSysMessage("Syntax: .arena 2 | .arena 3");
        return false;
    }
}

bool ChatHandler::HandleWarsongCommand(char * args)
{
    Player* player = m_session->GetPlayer();
    
    if (player->InBattleGround() || player->GetInstanceId())
    {
        SendSysMessage("You cannot use this command here.");
        return false;
    }

    if (player->getLevel() < 10)
    {
        SendSysMessage("Your level is not high enough.");
        return false;
    }
    return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_WS));
}

bool ChatHandler::HandleArathiCommand(char * args)
{
    Player* player = m_session->GetPlayer();

    if (player->InBattleGround() || player->GetInstanceId())
    {
        SendSysMessage("You cannot use this command here.");
        return false;
    }

    if (player->getLevel() < 20)
    {
        SendSysMessage("Your level is not high enough.");
        return false;
    }
    return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_AB));
}

bool ChatHandler::HandleAzsharaCommand(char* args)
{
    Player* player = m_session->GetPlayer();

    if (player->InBattleGround() || player->GetInstanceId())
    {
        SendSysMessage("You cannot use this command here.");
        return false;
    }

    if (player->getLevel() < 55)
    {
        SendSysMessage("Your level is not high enough.");
        return false;
    }
    return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_AC));
}

bool ChatHandler::HandleAlteracCommand(char * args)
{
    Player* player = m_session->GetPlayer();

    if (player->InBattleGround() || player->GetInstanceId())
    {
        SendSysMessage("You cannot use this command here.");
        return false;
    }

    if (player->getLevel() < 55)
    {
        SendSysMessage("Your level is not high enough.");
        return false;
    }
    return RegisterPlayerToBG(m_session, BattleGroundTypeId(BATTLEGROUND_AV));
}

bool ChatHandler::HandleExperienceCommand(char* args)
{
    Player* player = m_session->GetPlayer();

    if (!args)
    {
        SendSysMessage("Syntax: .experience on | off.");
        return true;
    }

    if (strcmp(args, "on") == 0)
    {
        SendSysMessage("Experience gain is now enabled.");
        player->EnableXP();
        return true;
    }
    else if (strcmp(args, "off") == 0)
    {

        SendSysMessage("Experience gain is now disabled.");
        player->DisableXP();
        return true;
    }

    SendSysMessage("Syntax: .experience on | off.");
    return true;
}

bool ChatHandler::HandleHonorPauseCommand(char* args)
{
    Player* player = m_session->GetPlayer();

    if (!args)
    {
        SendSysMessage("Syntax: .honorgain on | off.");
        return true;
    }

    if (strcmp(args, "on") == 0)
    {
        SendSysMessage("Honor gain is now enabled.");
        player->EnableHonorGain();
        return true;
    }
    else if (strcmp(args, "off") == 0)
    {

        SendSysMessage("Honor gain is now disabled.");
        player->DisableHonorGain();
        return true;
    }

    SendSysMessage("Syntax: .honorgain on | off.");
    return true;
}

bool ChatHandler::HandleHordeInstanceResetCommand(char* args)
{
    Player* player = m_session->GetPlayer();

    if (player->GetTeam() != HORDE)
    {
        SendSysMessage("You cannot use this command.");
        return false;
    }

    if (player->InBattleGround() || player->GetInstanceId())
    {
        SendSysMessage("You cannot use this command here.");
        return false;
    }

    if (player->HasAura(34669))
    {
        SendSysMessage("You can run that command only once per 12hrs.");
        return false;
    }

    if (!*args)
    {
        SendSysMessage("Syntax: .hordereset all");
        return false;
    }

    uint32 counter = 0;
    uint32 mapid = 0;
    bool got_map = false;

    if (strncmp(args, "all", strlen(args)) != 0)
    {
        if (!isNumeric(args[0]))
            return false;

        got_map = true;
        mapid = atoi(args);
    }

    Player::BoundInstancesMap &binds = player->GetBoundInstances();
    for (Player::BoundInstancesMap::iterator itr = binds.begin(); itr != binds.end();)
    {
        if (got_map && mapid != itr->first)
        {
            ++itr;
            continue;
        }
        if (itr->first != player->GetMapId())
        {
            DungeonPersistentState *save = itr->second.state;
            std::string timeleft = secsToTimeString(save->GetResetTime() - time(NULL), true);

            if (const MapEntry* entry = sMapStorage.LookupEntry<MapEntry>(itr->first))
            {
                PSendSysMessage("unbinding map: %d (%s) inst: %d perm: %s canReset: %s TTR: %s",
                    itr->first, entry->name, save->GetInstanceId(), itr->second.perm ? "yes" : "no",
                    save->CanReset() ? "yes" : "no", timeleft.c_str());
            }
            else
                PSendSysMessage("bound for a nonexistent map %u", itr->first);
            player->UnbindInstance(itr);
            counter++;
        }
        else
            ++itr;
    }
    PSendSysMessage("instances unbound: %d", counter);
    player->CastSpell(player, 34669, true);

    return true;
}

bool ChatHandler::HandleIronmanRestore(char* /*args*/)
{
    Unit* target = GetSelectedUnit();

    if (!target)
    {
        SendSysMessage("Target is required");
        SetSentErrorMessage(true);
        return false;
    }

    if (!target->IsPlayer())
    {
        SendSysMessage("Wrong target.");
        SetSentErrorMessage(true);
        return false;
    }

    if (!target->HasAura(34725))
    {
        target->AddAura(34725);
    }
    return true;
}