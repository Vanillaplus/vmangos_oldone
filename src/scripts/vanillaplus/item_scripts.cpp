  
#include "scriptPCH.h"

bool ItemUseSpell_item_character_rename(Player* pPlayer, Item* pItem, const SpellCastTargets&)
{
    if (!pPlayer) return false;
	
    ChatHandler(pPlayer).PSendSysMessage("Please logout and choose a new name.");
    pPlayer->SetAtLoginFlag(AT_LOGIN_RENAME);
    return true;
}

bool ItemUseSpell_item_summon_mailbox(Player* pPlayer, Item* pItem, const SpellCastTargets&)
{
	if (!pPlayer) return false;
	
    float dis{ 2.0F };
    float x, y, z;
    pPlayer->GetSafePosition(x, y, z);
    x += dis * cos(pPlayer->GetOrientation());
    y += dis * sin(pPlayer->GetOrientation());
    
    pPlayer->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
    float ground_z = pPlayer->GetMap()->GetHeight(x, y, z, true);
    pPlayer->SummonGameObject(144112, x, y, ground_z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 180, true);
    return true;
}

bool ItemUseSpell_item_brainwashing_device(Player* pPlayer, Item* pItem, const SpellCastTargets&)
{
    if (pPlayer->isInCombat() || pPlayer->IsBeingTeleported() || (pPlayer->getDeathState() == CORPSE) || pPlayer->IsMoving())
        ChatHandler(pPlayer).PSendSysMessage("Warning! Failsafe system shutting device down!");
    else
    {
        float dis{ 2.0F };
        float x, y, z;
        pPlayer->GetSafePosition(x, y, z);
        x += dis * cos(pPlayer->GetOrientation());
        y += dis * sin(pPlayer->GetOrientation());
        float ground_z = pPlayer->GetMap()->GetHeight(x, y, z, true);
        pPlayer->SummonGameObject(1000333, x, y, ground_z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 120, true);
        return true;
    }
    return false;
}

bool GOHello_go_brainwashing_device(Player* pPlayer, GameObject* pGo)
{
    if (pPlayer->getLevel() >= 10 && pPlayer->HasItemCount(51715, 1))
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, "Click here to reset your talents.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

    pPlayer->SEND_GOSSIP_MENU(90350, pGo->GetGUID());
    return true;
}

bool GOSelect_go_brainwashing_device(Player* pPlayer, GameObject* pGo, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF + 1)
    {
        pPlayer->ResetTalents();
        pPlayer->AddAura(27880);
        pGo->Despawn();
        pGo->UpdateObjectVisibility();
        pPlayer->CLOSE_GOSSIP_MENU();
    }
    return true;
}

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

bool ItemUseSpell_skin_changer(Player* pPlayer, Item* pItem, const SpellCastTargets&) 
{
    bool isMale = pPlayer->getGender() == GENDER_MALE;
    uint32 itemEntry = pItem->GetEntry();
    int8 bytesToSet = -1;

    switch (pPlayer->getRace()) 
    {
    case RACE_HUMAN:
        switch (itemEntry) 
        {
        case 60105: // Sally Whitemane
            if (!isMale)
                bytesToSet = 10;
            break;
        case 60106: // Cult of the Damned
            bytesToSet = 11;
            break;
        default:
            break;
        }
        break;
    case RACE_DWARF:
        switch (itemEntry) 
        {
        case 60204: // Wildhammer
            if (isMale)
                bytesToSet = static_cast<uint8>(irand(16, 17));
            else
                bytesToSet = 9;
            break;
        case 60205: // Dark Iron
            if (isMale)
                bytesToSet = 9;
            else
                bytesToSet = 10;
            break;
        case 60206: // Earthen
            if (isMale)
                bytesToSet = static_cast<uint8>(irand(10, 15));
            break;
        default:
            break;
        }
        break;
    case RACE_ORC:
        switch (itemEntry) 
        {
        case 60207: // Blackrock
            if (isMale)
                bytesToSet = static_cast<uint8>(irand(0, 1) == 0 ? 9 : 11);
            else
                bytesToSet = 9;
            break;
        case 60208: // Chaos
            /*if (isMale)*/
            bytesToSet = 10;
            break;
        case 60209: // Mag'Har
            if (isMale)
                bytesToSet = 12;
            break;
        default:
            break;
        }
        break;
    case RACE_TROLL:
        switch (itemEntry) 
        {
        case 60210: // Forest
            bytesToSet = 13;
            break;
        case 60211: // Sandfury (Farraki), 14 is the zombie one.
            bytesToSet = static_cast<uint8>(irand(0, 1) == 0 ? static_cast<uint8>(irand(9, 11)) : 14);
            break;
        case 61010: // Dark Troll
            bytesToSet = 6;
            break;
        case 61011: // Ice Troll
            bytesToSet = 7;
            break;
        default:
            break;
        }
        break;
    case RACE_GNOME:
        switch (itemEntry) 
        {
        case 60106: // Cult of the Damned
            if (isMale)
                bytesToSet = 6;
            else
                bytesToSet = 5;
            break;
        case 60212: // Leper
            if (isMale)
                bytesToSet = 5;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (bytesToSet > -1) 
    {
        pPlayer->SetByteValue(PLAYER_BYTES, 0, static_cast<uint8>(bytesToSet));
        pPlayer->SetDisplayId(15435); // Invisible
        pPlayer->m_Events.AddEvent(new DemorphAfterTime(pPlayer->GetGUID()), pPlayer->m_Events.CalculateTime(250));
    }
    else 
    {
        ChatHandler(pPlayer).SendSysMessage("You can't use this item.");
    }
    return false;
}

void AddSC_item_scripts()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "item_skin_change";
    newscript->pItemUseSpell = &ItemUseSpell_skin_changer;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_brainwashing_device";
    newscript->pGOHello = &GOHello_go_brainwashing_device;
    newscript->pGOGossipSelect = &GOSelect_go_brainwashing_device;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "item_brainwashing_device";
    newscript->pItemUseSpell = &ItemUseSpell_item_brainwashing_device;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "item_character_rename";
    newscript->pItemUseSpell = &ItemUseSpell_item_character_rename;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "item_summon_mailbox";
    newscript->pItemUseSpell = &ItemUseSpell_item_summon_mailbox;
    newscript->RegisterSelf();
}