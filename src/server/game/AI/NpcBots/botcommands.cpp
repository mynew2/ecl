/*
Name: script_bot_commands
%Complete: ???
Comment: Playerbot and Npcbot related commands
Category: commandscripts/custom/
*/

#include "bot_ai.h"
#include "bp_ai.h"
#include "bp_mgr.h"
#include "Chat.h"
#include "Config.h"
#include "Group.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"

class script_bot_commands : public CommandScript
{
public:
    script_bot_commands() : CommandScript("script_bot_commands") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand npcbotCommandTable[] =
        {
            { "info",           SEC_PLAYER,         false, &HandleNpcBotInfoCommand,                    "", NULL },
            { "add",            SEC_PLAYER,         false, &HandleNpcBotAddCommand,                     "", NULL },
            { "revive",         SEC_MODERATOR,      false, &HandleNpcBotReviveCommand,                  "", NULL },
            { "remove",         SEC_PLAYER,         false, &HandleNpcBotRemoveCommand,                  "", NULL },
            { "reset",          SEC_PLAYER,         false, &HandleNpcBotResetCommand,                   "", NULL },
            { "command",        SEC_PLAYER,         false, &HandleNpcBotCommandCommand,                 "", NULL },
            { "distance",       SEC_PLAYER,         false, &HandleNpcBotDistanceCommand,                "", NULL },
            //{ "reloadequips",   SEC_ADMINISTRATOR,  false, &HandleReloadEquipsCommand,                  "", NULL },
            { NULL,             0,                  false, NULL,                                        "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "bot",            SEC_ADMINISTRATOR,  false, &HandlePlayerbotCommand,                     "", NULL },
            { "maintank",       SEC_PLAYER,         false, &HandleMainTankCommand,                      "", NULL },
            { "mt",             SEC_PLAYER,         false, &HandleMainTankCommand,                      "", NULL },
            { "npcbot",         SEC_PLAYER,         false, NULL,                          "", npcbotCommandTable },
            { NULL,             0,                  false, NULL,                                        "", NULL }
        };
        return commandTable;
    }

    //static bool HandleReloadEquipsCommand(ChatHandler* handler, const char* /*args*/)
    //{
    //    TC_LOG_INFO(LOG_FILTER_GENERAL, "Re-Loading Creature Equips...");
    //    sObjectMgr->LoadEquipmentTemplates();
    //    handler->SendGlobalGMSysMessage("DB table `creature_equip_template` (creature equipment) reloaded.");
    //    return true;
    //}

    static bool HandlePlayerbotCommand(ChatHandler* handler, const char* args)
    {
        if (!handler->GetSession())
        {
            handler->PSendSysMessage("You may only add bots from an active session");
            handler->SetSentErrorMessage(true);
            return false;
        }

        bool allowPBots = ConfigMgr::GetBoolDefault("Bot.EnablePlayerBots", false);
        if (allowPBots == false)
        {
            handler->PSendSysMessage("Playerbot system is disabled");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!*args)
        {
            handler->PSendSysMessage("usage: add PLAYERNAME  or  remove PLAYERNAME");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* player = handler->GetSession()->GetPlayer();

        char* cmd = strtok ((char*)args, " ");
        if (!cmd)
        {
            handler->PSendSysMessage("usage: add PLAYERNAME  or  remove PLAYERNAME");
            handler->SetSentErrorMessage(true);
            return false;
        }
        std::string cmdStr = cmd;
        
        //if (cmdStr.compare("tele") == 0 || cmdStr.compare("teleport") == 0 || cmdStr.compare("summ") == 0 || cmdStr.compare("summon") == 0)
        //{
        //    if (handler->GetSession()->m_playerBots.empty())
        //    {
        //        handler->PSendSysMessage("You Have No Playerbots!");
        //        handler->SetSentErrorMessage(true);
        //        return false;
        //    }
        //    PlayerbotChatHandler ch(player);
        //    for (PlayerBotMap::const_iterator itr = handler->GetSession()->GetPlayerBotsBegin(); itr != handler->GetSession()->GetPlayerBotsEnd(); ++itr)
        //    {
        //        Player* botPlayer = itr->second;
        //        if (!botPlayer)
        //            continue;
        //        ch.teleport(*botPlayer);
        //        //botPlayer->TeleportTo(*player);
        //    }
        //    return true;
        //}
        char* charname = strtok (NULL, " ");
        if (!charname)
        {
            handler->PSendSysMessage("usage: add PLAYERNAME  or  remove PLAYERNAME");
            handler->SetSentErrorMessage(true);
            return false;
        }
        std::string charnameStr = charname;

        PlayerbotMgr* mgr = player->GetPlayerbotMgr();
        if (!mgr)
        {
            mgr = new PlayerbotMgr(player);
            player->SetPlayerbotMgr(mgr);
        }

        uint64 guid;

       if (charnameStr.compare("all") != 0)
       {
           if (!normalizePlayerName(charnameStr))
               return false;

           guid = sObjectMgr->GetPlayerGUIDByName(charnameStr.c_str());
           if (guid == 0 || (guid == handler->GetSession()->GetPlayer()->GetGUID()))
           {
               handler->PSendSysMessage(LANG_PLAYER_NOT_FOUND);
               handler->SetSentErrorMessage(true);
               return false;
           }

           uint32 accountId = sObjectMgr->GetPlayerAccountIdByGUID(guid);
           if (accountId != handler->GetSession()->GetAccountId())
           {
               handler->PSendSysMessage("You may only add bots from the same account.");
               handler->SetSentErrorMessage(true);
               return false;
           }
       }

        if (cmdStr.compare("add") == 0 || cmdStr.compare("login") == 0)
        {
            if (charnameStr.compare("all") == 0)
            {
                std::string plName;
                std::list<std::string>* names = new std::list<std::string>;
                uint32 accId = player->GetSession()->GetAccountId();
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYERBOTS);
                stmt->setUInt32(0, accId);
                stmt->setUInt32(1, player->GetGUIDLow());
                PreparedQueryResult results = CharacterDatabase.Query(stmt);
                //CharacterDatabase.PQuery("SELECT name FROM characters WHERE account = '%u' AND guid != '%u'", accId, player->GetGUIDLow());
                if (results)
                {
                    do
                    {
                        Field* fields = results->Fetch();
                        plName = fields[0].GetString();
                        if (ObjectAccessor::FindPlayerByName(plName))
                            continue;
                        names->insert(names->end(), plName);
                    } while(results->NextRow());
                }
                std::list<std::string>::iterator iter,next;
                for (iter = names->begin(); iter != names->end(); iter++)
                {
                    std::stringstream arg;
                    arg << "add " << (*iter).c_str();
                    HandlePlayerbotCommand(handler, arg.str().c_str());
                }
                handler->PSendSysMessage("Bots added successfully.");
                delete names;
                return true;
            }
            else
            {
                guid = sObjectMgr->GetPlayerGUIDByName(charnameStr.c_str());
                if (mgr->GetPlayerBot(guid) != NULL)
                {
                    handler->PSendSysMessage("Bot already exists in world.");
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                mgr->AddPlayerBot(guid);
            }
        }
        else if (cmdStr.compare("remove") == 0 || cmdStr.compare("logout") == 0)
        {
            if (charnameStr.compare("all") == 0)
            {
                std::list<std::string>* names = new std::list<std::string>;
                for (PlayerBotMap::const_iterator iter = mgr->GetPlayerBotsBegin(); iter != mgr->GetPlayerBotsEnd(); ++iter)
                {
                    names->push_back(iter->second->GetName());
                }
                std::list<std::string>::iterator iter, next;
                for (iter = names->begin(); iter != names->end(); iter++)
                {
                    std::stringstream arg;
                    arg << "remove " << (*iter).c_str();
                    HandlePlayerbotCommand(handler, arg.str().c_str());
                }
                delete names;
                return true;
            }
            else
            {
                guid = sObjectMgr->GetPlayerGUIDByName(charnameStr.c_str());
                if (!mgr->GetPlayerBot(guid))
                {
                    handler->PSendSysMessage("Bot can not be removed because bot does not exist in world.");
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                mgr->LogoutPlayerBot(guid);
                handler->PSendSysMessage("Bot removed successfully.");
                return true;
            }
        }
        else if (cmdStr == "co" || cmdStr == "combatorder")
        {
            Unit* target = NULL;
            char* orderChar = strtok(NULL, " ");
            if (!orderChar || mgr->getPlayerbots().empty())
            {
                handler->PSendSysMessage("|cffff0000Syntax error:|cffffffff .bot co <botName> <order=reset|tank|assist|heal|protect> [targetPlayer]");
                handler->SetSentErrorMessage(true);
                return false;
            }
            std::string orderStr = orderChar;
            if (orderStr == "protect" || orderStr == "assist")
            {
                char* targetChar = strtok(NULL, " ");
                uint64 targetGUID = handler->GetSession()->GetPlayer()->GetSelection();
                if (!targetChar && !targetGUID)
                {
                    handler->PSendSysMessage("|cffff0000Combat orders protect and assist expect a target either by selection or by giving target player in command string!");
                    handler->SetSentErrorMessage(true);
                    return false;
                }
                if (targetChar)
                {
                    std::string targetStr = targetChar;
                    targetGUID = sObjectMgr->GetPlayerGUIDByName(targetStr.c_str());
                }
                target = ObjectAccessor::GetUnit(*handler->GetSession()->GetPlayer(), targetGUID);
                if (!target)
                {
                    handler->PSendSysMessage("|cffff0000Invalid target for combat order protect or assist!");
                    handler->SetSentErrorMessage(true);
                    return false;
                }
            }
            //if (handler->GetSession()->GetPlayerBot(guid) == NULL)
            //{
            //    handler->PSendSysMessage("|cffff0000Bot can not receive combat order because bot does not exist in world.");
            //    handler->SetSentErrorMessage(true);
            //    return false;
            //}
            //if (mgr)
            //for (PlayerBotMap::const_iterator itr = mgr->GetPlayerBotsBegin(); itr != mgr->GetPlayerBotsEnd(); ++itr)
            //    if (Player* bot = ObjectAccessor::GetPlayer(*player, itr->first))
            //        bot->GetPlayerbotAI()->SetCombatOrderByStr(orderStr, target);
        }
        return true;
    }

    static bool HandleMainTankCommand(ChatHandler* handler, const char* args)
    {
        Group* group = handler->GetSession()->GetPlayer()->GetGroup();
        if (!group)
        {
            handler->PSendSysMessage("Must be in a group to use main tank command.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        uint64 myguid = handler->GetSession()->GetPlayer()->GetGUID();
        if (!group->IsLeader(myguid) && !group->IsAssistant(myguid))
        {
            handler->PSendSysMessage("you have no permission to set main tank.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!*args)
        {
            if (uint64 selection = handler->GetSession()->GetPlayer()->GetSelection())
            {
                if (group->IsMember(selection))
                {
                    if (Unit* u = ObjectAccessor::FindUnit(selection))
                    {
                        bool isabot = u->GetTypeId() == TYPEID_UNIT && u->ToCreature()->GetIAmABot();
                        if (isabot && group->GetMemberSlots().size() < 3 && handler->GetSession()->GetSecurity() == SEC_PLAYER)
                        {
                            handler->PSendSysMessage("Your party is too small to set a npcbot main tank.");
                            handler->SetSentErrorMessage(true);
                            return false;
                        }
                        group->RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINTANK);
                        Group::MemberSlotList const& members = group->GetMemberSlots();
                        for (Group::MemberSlotList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
                        {
                            uint8 flags = itr->flags;
                            if (group->isRaidGroup())
                            {
                                //try to set flags in group (will fail if not raid)
                                group->SetGroupMemberFlag(itr->guid, itr->guid == selection, MEMBER_FLAG_MAINTANK);
                            }
                            else //force flags for non-raid group (DB only) this will allow bots to find tank
                            {
                                if (itr->guid == selection && !(flags & MEMBER_FLAG_MAINTANK))
                                    flags |= MEMBER_FLAG_MAINTANK;
                            }
                            //store result if DB
                            if (itr->guid != selection || !group->isRaidGroup())
                            {
                                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_FLAG);
                                stmt->setUInt8(0, flags);
                                stmt->setUInt32(1, GUID_LOPART(itr->guid));
                                CharacterDatabase.Execute(stmt);
                            }
                            //send result to players and their bots
                            if (!IS_PLAYER_GUID(itr->guid))
                                continue;
                            if (Player* player = ObjectAccessor::FindPlayer(itr->guid))
                            {
                                ChatHandler chp(player->GetSession());
                                chp.PSendSysMessage("Main tank is set to %s.", u->GetName().c_str());
                                player->SetBotTank(selection);
                                if (player->HaveBot())
                                {
                                    for (uint8 i = 0; i != player->GetMaxNpcBots(); ++i)
                                    {
                                        Creature* cre = player->GetBotMap(i)->_Cre();
                                        if (cre)
                                            cre->SetBotTank(u);
                                    }
                                }
                            }
                        }
                        u->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                        handler->SetSentErrorMessage(true);
                        return true;
                    }
                }
            }
            if (Unit* unit = bot_ai::GetBotGroupMainTank(group))
            {
                bool bot = unit->GetTypeId() == TYPEID_UNIT && unit->ToCreature()->GetIAmABot();
                handler->PSendSysMessage("Main tank is %s (%s%s).", unit->GetName().c_str(), (bot ? "npcbot" : "player"), (unit->isAlive() ? "" : ", dead"));
                handler->SetSentErrorMessage(true);
                return true;
            }
            handler->PSendSysMessage(".maintank");
            handler->PSendSysMessage("Allows to set a main tank in bot party (can be used on npcbots). Determines npcbots' actions");
            handler->PSendSysMessage("Npcbot maintank also receives damage reduction, avoidance and threat generation bonus");
            handler->SetSentErrorMessage(true);
            return true;
        }
        else
        {
            //clear tank in whole bot party
            std::string cmdStr = strtok((char*)args, " ");
            if (!cmdStr.compare("clear") || !cmdStr.compare("cl") || !cmdStr.compare("cle") ||
                !cmdStr.compare("reset") || !cmdStr.compare("res"))
            {
                Group::MemberSlotList const& members = group->GetMemberSlots();
                for (Group::MemberSlotList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
                {
                    uint8 flags = itr->flags;
                    if (group->isRaidGroup())
                    {
                        if (flags & MEMBER_FLAG_MAINTANK)
                            group->SetGroupMemberFlag(itr->guid, false, MEMBER_FLAG_MAINTANK);
                    }
                    else
                    {
                        if (itr->flags & MEMBER_FLAG_MAINTANK)
                            flags &= ~MEMBER_FLAG_MAINTANK;
                    }
                    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_GROUP_MEMBER_FLAG);
                    stmt->setUInt8(0, flags);
                    stmt->setUInt32(1, GUID_LOPART(itr->guid));
                    CharacterDatabase.Execute(stmt);
                    if (!IS_PLAYER_GUID(itr->guid))
                        continue;
                    Player* player = ObjectAccessor::FindPlayer(itr->guid);
                    if (!player) continue;
                    ChatHandler(player->GetSession()).PSendSysMessage("Main tank has been reset by %s.", handler->GetSession()->GetPlayer()->GetName().c_str());
                    player->SetBotTank(0);
                    if (player->HaveBot())
                    {
                        for (uint8 i = 0; i != player->GetMaxNpcBots(); ++i)
                        {
                            Creature* cre = player->GetBotMap(i)->_Cre();
                            if (cre)
                                cre->SetBotTank(NULL);
                        }
                    }
                }
                handler->SetSentErrorMessage(true);
                return true;
            }
        }
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleNpcBotInfoCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* owner = handler->GetSession()->GetPlayer();
        if (!owner->GetSelection())
        {
            handler->PSendSysMessage(".npcbot info");
            handler->PSendSysMessage("Lists NpcBots count of each class owned by selected player. You can use this on self and your Playerbots");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* master = owner->GetSelectedPlayer();
        if (!master || (master && master != owner && master->GetSession()->m_master != owner))
        {
            handler->PSendSysMessage("You should select one of your PlayerBots or self.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        //create list
        std::set<Player*> Players;
        Players.insert(master);
        PlayerbotMgr* mgr = master->GetPlayerbotMgr();
        if (mgr && !mgr->getPlayerbots().empty())
            for (PlayerBotMap::const_iterator itr = mgr->GetPlayerBotsBegin(); itr != mgr->GetPlayerBotsEnd(); ++itr)
                Players.insert(itr->second);
        //cycle through
        for (std::set<Player*>::const_iterator it = Players.begin(); it != Players.end(); ++it)
        {
            Player* pl = *it;
            if (!pl->HaveBot())
            {
                handler->PSendSysMessage("%s has no NpcBots!", pl->GetName().c_str());
                continue;
            }
            const std::string plType = pl == owner ? ", player" : ", playerbot";
            handler->PSendSysMessage("Listing NpcBots for %s%s", pl->GetName().c_str(), plType.c_str());
            handler->PSendSysMessage("Owned NpcBots: %u", pl->GetNpcBotsCount());
            for (uint8 i = CLASS_WARRIOR; i != MAX_CLASSES; ++i)
            {
                uint8 count = 0;
                uint8 alivecount = 0;
                for (uint8 pos = 0; pos != pl->GetMaxNpcBots(); ++pos)
                {
                    if (Creature* cre = pl->GetBotMap(pos)->_Cre())
                    {
                        if (cre->GetBotClass() == i)
                        {
                            ++count;
                            if (cre->isAlive())
                                ++alivecount;
                        }
                    }
                }
                char const* bclass;
                switch (i)
                {
                case CLASS_WARRIOR:         bclass = "Warriors";        break;
                case CLASS_PALADIN:         bclass = "Paladins";        break;
                case CLASS_MAGE:            bclass = "Mages";           break;
                case CLASS_PRIEST:          bclass = "Priests";         break;
                case CLASS_WARLOCK:         bclass = "Warlocks";        break;
                case CLASS_DRUID:           bclass = "Druids";          break;
                case CLASS_DEATH_KNIGHT:    bclass = "DeathKnights";    break;
                case CLASS_ROGUE:           bclass = "Rogues";          break;
                case CLASS_SHAMAN:          bclass = "Shamans";         break;
                case CLASS_HUNTER:          bclass = "Hunters";         break;
                default:                    bclass = "Unknown Class";   break;
                }
                if (count > 0)
                    handler->PSendSysMessage("%s: %u (alive: %u)", bclass, count, alivecount);
            }
        }
        return true;
    }

    static bool HandleNpcBotDistanceCommand(ChatHandler* handler, const char* args)
    {
        Player* owner = handler->GetSession()->GetPlayer();
        if (!*args)
        {
            if (owner->HaveBot())
            {
                handler->PSendSysMessage("bot follow distance is %u", owner->GetBotFollowDist());
                handler->SetSentErrorMessage(true);
                return false;
            }
            handler->PSendSysMessage(".npcbot distance");
            handler->PSendSysMessage("Sets 'distance to target' at which bots will follow you");
            handler->PSendSysMessage("if set to 0, bots will not attack anything unless you point them");
            handler->PSendSysMessage("min: 0, max: 75");
            handler->SetSentErrorMessage(true);
            return false;
        }
        char* distance = strtok((char*)args, " ");
        int8 dist = -1;

        if (distance)
            dist = (int8)atoi(distance);

        if (dist >= 0 && dist <= 75)
        {
            owner->SetBotFollowDist(dist);
            if (!owner->isInCombat() && owner->HaveBot())
            {
                for (uint8 i = 0; i != owner->GetMaxNpcBots(); ++i)
                {
                    Creature* cre = owner->GetBotMap(i)->_Cre();
                    if (!cre || !cre->IsInWorld()) continue;
                    owner->SendBotCommandState(cre, COMMAND_FOLLOW);
                }
            }
            PlayerbotMgr* mgr = owner->GetPlayerbotMgr();
            if (mgr && !mgr->getPlayerbots().empty())
            {
                for (PlayerBotMap::const_iterator itr = mgr->GetPlayerBotsBegin(); itr != mgr->GetPlayerBotsEnd(); ++itr)
                {
                    if (Player* bot = itr->second)
                    {
                        bot->SetBotFollowDist(dist);
                        if (!bot->isInCombat() && bot->HaveBot())
                        {
                            for (uint8 i = 0; i != bot->GetMaxNpcBots(); ++i)
                            {
                                Creature* cre = bot->GetBotMap(i)->_Cre();
                                if (!cre || !cre->IsInWorld()) continue;
                                bot->SendBotCommandState(cre, COMMAND_FOLLOW);
                            }
                        }
                    }
                }
            }
            handler->PSendSysMessage("bot follow distance set to %u", dist);
            return true;
        }
        handler->SendSysMessage("follow distance should be between 0 and 75");
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleNpcBotCommandCommand(ChatHandler* handler, const char* args)
    {
        Player* owner = handler->GetSession()->GetPlayer();
        if (!*args)
        {
            handler->PSendSysMessage(".npcbot command <command>");
            handler->PSendSysMessage("Forces npcbots to either follow you or hold position. Can be used on Playerbots");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* master = owner->GetSelectedPlayer();
        if (!master || (master && master != owner && master->GetSession()->m_master != owner))
            master = owner;
        char* command = strtok((char*)args, " ");
        int8 state = -1;
        if (!strncmp(command, "s", 2) || !strncmp(command, "st", 3) || !strncmp(command, "stay", 5) || !strncmp(command, "stand", 6))
            state = COMMAND_STAY;
        else if (!strncmp(command, "f", 2) || !strncmp(command, "follow", 7) || !strncmp(command, "fol", 4) || !strncmp(command, "fo", 3))
            state = COMMAND_FOLLOW;
        if (state >= 0 && master->HaveBot())
        {
            for (uint8 i = 0; i != master->GetMaxNpcBots(); ++i)
            {
                Creature* cre = master->GetBotMap(i)->_Cre();
                if (!cre || !cre->IsInWorld()) continue;
                master->SendBotCommandState(cre, CommandStates(state));
            }
            return true;
        }
        return false;
    }

    static bool HandleNpcBotRemoveCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* owner = handler->GetSession()->GetPlayer();
        uint64 guid = owner->GetSelection();
        if (!guid)
        {
            handler->PSendSysMessage(".npcbot remove");
            handler->PSendSysMessage("Remove npcbots for selected Playerbot, you can also remove npcbots manually");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* master = ObjectAccessor::GetPlayer(*owner, guid);
        if (master)
        {
            if (master != owner && master->GetSession()->m_master != owner)
            {
                handler->PSendSysMessage("You can only remove bots from self and your own playerbots!");
                handler->SetSentErrorMessage(true);
                return false;
            }
            if (master->HaveBot())
            {
                for (uint8 i = 0; i != master->GetMaxNpcBots(); ++i)
                {
                    master->RemoveBot(master->GetBotMap(i)->_Guid(), true);
                }
                if (!master->HaveBot())
                {
                    handler->PSendSysMessage("Npcbots successfully removed");
                    handler->SetSentErrorMessage(true);
                    return true;
                }
                handler->PSendSysMessage("Error!");
                handler->SetSentErrorMessage(true);
                return false;
            }
            handler->PSendSysMessage("Npcbots are not found!");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Creature* cre = ObjectAccessor::GetCreature(*owner, guid);
        if (cre && cre->GetIAmABot())
        {
            master = cre->GetBotOwner();
            if (!master || (master && master != owner && master->GetSession()->m_master != owner))
            {
                handler->PSendSysMessage("You can only remove bots from self and your own playerbots");
                handler->SetSentErrorMessage(true);
                return false;
            }
            uint8 pos = master->GetNpcBotSlot(guid);
            master->RemoveBot(cre->GetGUID(), true);
            if (master->GetBotMap(pos)->_Cre() == NULL)
            {
                handler->PSendSysMessage("NpcBot successfully removed");
                handler->SetSentErrorMessage(true);
                return true;
            }
            handler->PSendSysMessage("NpcBot is NOT removed for some reason!");
            handler->SetSentErrorMessage(true);
            return false;
        }
        handler->PSendSysMessage("You should select Player or it's Npcbot!");
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleNpcBotResetCommand(ChatHandler* handler, const char* /*args*/)
    {
        Player* owner = handler->GetSession()->GetPlayer();
        Player* master = NULL;
        bool all = false;
        uint64 guid = owner->GetSelection();
        if (!guid)
        {
            handler->PSendSysMessage(".npcbot reset");
            handler->PSendSysMessage("Reset selected npcbot or npcbots for selected Playerbot, you can also reset your npcbots");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (IS_PLAYER_GUID(guid))
        {
            master = ObjectAccessor::FindPlayer(guid);
            all = true;
        }
        else if (IS_CREATURE_GUID(guid))
        {
            if (Creature* cre = ObjectAccessor::GetCreature(*owner, guid))
                master = cre->GetBotOwner();
        }
        if (master && (master == owner || master->GetSession()->m_master == owner))
        {
            if (master->isInCombat() && master->GetSession()->GetSecurity() == SEC_PLAYER)
            {
                handler->PSendSysMessage("Cannot reset bots in combat!");
                handler->SetSentErrorMessage(true);
                return false;
            }
            if (!master->HaveBot())
            {
                handler->PSendSysMessage("Npcbots are not found!");
                handler->SetSentErrorMessage(true);
                return false;
            }
            for (uint8 i = 0; i != master->GetMaxNpcBots(); ++i)
            {
                if (all)
                    master->RemoveBot(master->GetBotMap(i)->_Guid());
                else if (master->GetBotMap(i)->_Guid() == guid)
                {
                    master->RemoveBot(guid);
                    break;
                }
            }
            return true;
        }
        handler->PSendSysMessage(".npcbot reset");
        handler->PSendSysMessage("Reset selected npcbot or npcbot for selected Playerbot, you can also reset your npcbot. Cannot be used in combat");
        handler->SetSentErrorMessage(true);
        return false;
    }
    //For debug purposes only
    static bool HandleNpcBotReviveCommand(ChatHandler* handler, const char* /*args*/)
    {
        if (handler->GetSession()->GetSecurity() == SEC_PLAYER)
        {
            handler->PSendSysMessage("Revive command is disabled");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* owner = handler->GetSession()->GetPlayer();
        if (owner->InBattleground())
        {
            handler->PSendSysMessage("Bot revival is disabled in pvp matches");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (owner->isInCombat())
        {
            handler->PSendSysMessage("Bot revival is disabled in combat");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (owner->isInFlight())
        {
            handler->PSendSysMessage("Bot revival is disabled in flight");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (owner->HaveBot())
        {
            for (uint8 i = 0; i != owner->GetMaxNpcBots(); ++i)
            {
                Creature* bot = owner->GetBotMap(i)->_Cre();
                if (!bot) continue;
                if (bot->isDead())
                {
                    owner->SetBot(bot);
                    owner->CreateBot(0, 0, 0, false, true);
                }
            }
            handler->PSendSysMessage("NpcBots revived");
            handler->SetSentErrorMessage(true);
            return true;
        }
        handler->PSendSysMessage(".npcbot revive");
        handler->PSendSysMessage("Revive your npcbots if you are all hopelessly dead");
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleNpcBotAddCommand(ChatHandler* handler, const char* args)
    {
        Player* owner = handler->GetSession()->GetPlayer();
        Player* master = owner->GetSelectedPlayer();
        if (!master || !*args || (master != owner && master->GetSession()->m_master != owner))
        {
            handler->PSendSysMessage(".npcbot add");
            handler->PSendSysMessage("Allows to create npcbot of given class for targeted Playerbot, can be also used on self");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (master->RestrictBots())
        {
            handler->GetSession()->SendNotification("This place is restricted for NpcBots");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (master->isDead())
        {
            if (master == owner)
                owner->GetSession()->SendNotification("You're dead!");
            else
                owner->GetSession()->SendNotification("%s is dead!", master->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (master->GetGroup() && master->GetGroup()->isRaidGroup() && master->GetGroup()->IsFull())
        {
            handler->PSendSysMessage("Group is full, aborted");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (master->GetNpcBotsCount() >= master->GetMaxNpcBots())
        {
            handler->PSendSysMessage("NpcBots limit exceed");
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* bclass = strtok((char*)args, " ");
        uint8 botclass = CLASS_NONE;

        if (!strncmp(bclass, "deathknight", 12) || !strncmp(bclass, "dk", 3) || !strncmp(bclass, "de", 3))
            botclass = CLASS_DEATH_KNIGHT;
        else if (!strncmp(bclass, "druid", 6) || !strncmp(bclass, "dru", 4) || !strncmp(bclass, "dr", 3))
            botclass = CLASS_DRUID;
        else if (!strncmp(bclass, "hunter", 7) || !strncmp(bclass, "hunt", 5) || !strncmp(bclass, "hu", 3))
            botclass = CLASS_HUNTER;
        else if (!strncmp(bclass, "mage", 5) || !strncmp(bclass, "ma", 3))
            botclass = CLASS_MAGE;
        else if (!strncmp(bclass, "paladin", 8) || !strncmp(bclass, "pal", 4) || !strncmp(bclass, "pa", 3))
            botclass = CLASS_PALADIN;
        else if (!strncmp(bclass, "priest", 7) || !strncmp(bclass, "pri", 4) || !strncmp(bclass, "pr", 3))
            botclass = CLASS_PRIEST;
        else if (!strncmp(bclass, "rogue", 6) || !strncmp(bclass, "rog", 4) || !strncmp(bclass, "ro", 3))
            botclass = CLASS_ROGUE;
        else if (!strncmp(bclass, "shaman", 7) || !strncmp(bclass, "sha", 4) || !strncmp(bclass, "sh", 3))
            botclass = CLASS_SHAMAN;
        else if (!strncmp(bclass, "warlock", 8) || !strncmp(bclass, "warl", 5) || !strncmp(bclass, "lock", 5))
            botclass = CLASS_WARLOCK;
        else if (!strncmp(bclass, "warrior", 8) || !strncmp(bclass, "warr", 5))
            botclass = CLASS_WARRIOR;

        if (botclass == CLASS_NONE)
        {
            handler->PSendSysMessage("Wrong bot class");
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint8 bots = master->GetNpcBotsCount();
        master->CreateNPCBot(botclass);
        master->RefreshBot(0);
        if (master->GetNpcBotsCount() > bots)
        {
            if (master->isInCombat())
                handler->PSendSysMessage("NpcBot successfully created (%s). Will appear out of combat", master->GetName().c_str());
            else
                handler->PSendSysMessage("NpcBot successfully created (%s).", master->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return true;
        }
        handler->PSendSysMessage("NpcBot is NOT created for some reason!");
        handler->SetSentErrorMessage(true);
        return false;
    }
};

void AddSC_script_bot_commands()
{
    new script_bot_commands();
}
