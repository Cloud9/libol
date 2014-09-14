// Copyright (c) 2014 Andrew Toulouse.
// Distributed under the MIT License.

#ifndef __libol__PacketDecoders__
#define __libol__PacketDecoders__

#include "Value.h"
#include "Block.h"
#include "PacketTypes.h"

#include <cstdint>
#include <cassert>

namespace libol {
    class SetAbilityLevelPkt {
    public:
        static std::string name() { return "SetAbilityLevel"; }

        static bool test(Block& block) {
            return block.type == PacketType::SetAbilityLevel && block.size == 0x3;
        }

        static Value decode(Block& block) {
            Object data = Object();

            data.setv("abilityId", block.content[0]);
            data.setv("level", block.content[1]);

            assert(block.content[2] == 0x00);

            return Value::create(data);
        }
    };

    class ExpGainPkt {
    public:
        static std::string name() { return "ExpGain"; }

        static bool test(Block& block) {
            return block.type == PacketType::ExpGain && block.size == 0xc;
        }

        static Value decode(Block& block) {
            Object data = Object();

            auto stream = block.createStream();

            data.setv("receiverEnt", stream.read<uint32_t>());
            data.setv("killedEnt", stream.read<uint32_t>());
            data.setv("amount", stream.read<float>());

            return Value::create(data);
        }
    };

    class GoldGainPkt {
    public:
        static std::string name() { return "GoldGain"; }

        static bool test(Block& block) {
            return block.type == PacketType::GoldGain && block.size == 0x8;
        }

        static Value decode(Block& block) {
            Object data = Object();

            auto stream = block.createStream();

            data.setv("receiverEnt", stream.read<uint32_t>());
            data.setv("amount", stream.read<float>());

            return Value::create(data);
        }
    };

    class SetInventoryPkt {
    public:
        static std::string name() { return "SetInventory"; }

        static bool test(Block& block) {
            return block.type == 0xFE && block.size == 0x98 && block.content[0] == 0x0C && block.content[1] == 0x01;
        }

        static Value decode(Block& block) {
            Object data = Object();
            std::array<Object, 10> items;

            auto stream = block.createStream(0x2);

            for(size_t i = 0; i < items.size(); i++) {
                items[i].setv("itemId", stream.read<uint32_t>());
                items[i].setv("slotId", stream.get());
                items[i].setv("quantity", stream.get());
                items[i].setv("charges", stream.get());
            }

            for(size_t i = 0; i < items.size(); i++) {
                items[i].setv("cooldown", stream.read<float>());
            }

            for(size_t i = 0; i < items.size(); i++) {
                items[i].setv("baseCooldown", stream.read<float>());
            }

            Array itemsArr = Array();
            for(auto& item : items)
                itemsArr.pushv(item);
            data.setv("items", itemsArr);

            return Value::create(data);
        }
    };

    class ItemPurchasePkt {
    public:
        static std::string name() { return "ItemPurchase"; }

        static bool test(Block& block) {
            return block.type == PacketType::ItemPurchase && block.size == 0x8;
        }

        static Value decode(Block& block) {
            Object data = Object();

            auto stream = block.createStream();

            data.setv("itemId", stream.read<uint32_t>());
            data.setv("slot", stream.get());
            data.setv("quantity", stream.read<uint16_t>());

            assert(block.content[0x7] == 0x40);

            return Value::create(data);
        }
    };

    class HeroSpawnPkt {
    public:
        static std::string name() { return "HeroSpawn"; }

        static bool test(Block& block) {
            return block.type == PacketType::HeroSpawn && block.size == 0xC3;
        }

        static Value decode(Block& block) {
            Object data = Object();

            auto stream = block.createStream();

            data.setv("entityId", stream.read<uint32_t>());
            data.setv("clientId", stream.read<uint32_t>());

            stream.ignore(0xA); // unknown

            // Summoner name
            char summonerChars[0x81];
            stream.read(summonerChars, 0x80);
            summonerChars[0x80] = 0x00;
            data.setv("summonerName", std::string(summonerChars));

            // Champion name
            char championChars[0x11];
            stream.read(championChars, 0x10);
            championChars[0x10] = 0x00;
            data.setv("championName", std::string(championChars));

            stream.ignore(0x21); // unknown

            return Value::create(data);
        }
    };

    class SummonerDataPkt {
    public:
        static std::string name() { return "SummonerData"; }

        static bool test(Block& block) {
            return block.type == PacketType::SummonerData && block.size == 0x212;
        }

        static Value decode(Block& block) {
            Object data = Object();

            auto stream = block.createStream();

            Array runes = Array();
            for(size_t i = 0; i < 30; i++) {
                runes.pushv(stream.read<uint32_t>());
            }
            data.setv("runes", runes);

            data.setv("spell1", stream.read<uint32_t>());
            data.setv("spell2", stream.read<uint32_t>());

            Array masteries = Array();
            size_t masteryCount = 0;
            while(masteryCount++ < 79) {
                auto pos = stream.tellg();
                stream.ignore(2);
                uint8_t test = stream.get();
                stream.seekg(pos);
                if(test == 0x03) break;

                Object entry = Object();
                entry.setv("id", stream.get());
                entry.setv("tree", stream.get());
                uint8_t byte = stream.get();
                assert(byte == 0x03);
                byte = stream.get();
                assert(byte == 0x00);
                entry.setv("pointsSpent", stream.get());
                masteries.pushv(entry);
            }
            data.setv("masteries", masteries);

            data.setv("level", block.content[0x210]);

            return Value::create(data);
        }
    };

    class PlayerStatsPkt {
    public:
        static std::string name() { return "PlayerStats"; }

        static bool test(Block& block) {
            return block.type == PacketType::PlayerStats;
        }

        static Value decode(Block& block) {
            bool hasJungleStats;
            switch(block.size) {
                case 0x120:
                    hasJungleStats = false;
                    break;
                case 0x128:
                    hasJungleStats = true;
                    break;
                default:
                    assert(false); // TODO
            }

            Object data = Object();

            auto stream = block.createStream();

            data.setv("assists", stream.read<uint32_t>());
            stream.ignore(0x4);
            data.setv("kills", stream.read<uint32_t>());
            stream.ignore(0x4);
            data.setv("doubleKills", stream.read<uint32_t>());
            stream.ignore(3 * 0x4);
            data.setv("unrealKills", stream.read<uint32_t>());
            data.setv("goldEarned", stream.read<float>());
            data.setv("goldSpent", stream.read<float>());
            stream.ignore(10 * 0x4);
            data.setv("currentKillingSpree", stream.read<uint32_t>());
            data.setv("largestCriticalStrike", stream.read<float>());
            data.setv("largestKillingSpree", stream.read<uint32_t>());
            data.setv("largestMultiKill", stream.read<uint32_t>());
            stream.ignore(0x4);
            data.setv("longestTimeSpentLiving", stream.read<float>());
            data.setv("magicDamageDealt", stream.read<float>());
            data.setv("magicDamageDealtToChampions", stream.read<float>());
            data.setv("magicDamageTaken", stream.read<float>());
            data.setv("minionsKilled", stream.read<uint32_t>());
            stream.ignore(0x2); // Padding
            data.setv("neutralMinionsKilled", stream.read<uint32_t>());
            if(hasJungleStats) {
                data.setv("neutralMinionsKilledInEnemyJungle", stream.read<uint32_t>());
                data.setv("neutralMinionsKilledInTeamJungle", stream.read<uint32_t>());
            }
            data.setv("deaths", stream.read<uint32_t>());
            data.setv("pentaKills", stream.read<uint32_t>());
            data.setv("physicalDamageDealt", stream.read<float>());
            data.setv("physicalDamageDealtToChampions", stream.read<float>());
            data.setv("physicalDamageTaken", stream.read<float>());
            stream.ignore(0x4);
            data.setv("quadraKills", stream.read<uint32_t>());
            stream.ignore(9 * 0x4);
            data.setv("teamId", stream.read<uint32_t>());
            stream.ignore(3 * 0x4);
            data.setv("totalDamageDealt", stream.read<float>());
            data.setv("totalDamageDealtToChamptions", stream.read<float>());
            data.setv("totalDamageTaken", stream.read<float>());
            data.setv("totalHeal", stream.read<uint32_t>());
            data.setv("totalTimeCrowdControlDealt", stream.read<float>());
            data.setv("totalTimeSpentDead", stream.read<float>());
            data.setv("totalUnitsHealed", stream.read<uint32_t>());
            data.setv("tripleKills", stream.read<uint32_t>());
            data.setv("trueDamageDealt", stream.read<float>());
            data.setv("trueDamageDealtToChamptions", stream.read<float>());
            data.setv("trueDamageTaken", stream.read<float>());
            data.setv("towerKills", stream.read<uint32_t>());
            data.setv("inhibitorKills", stream.read<uint32_t>());
            stream.ignore(0x4);
            data.setv("wardsKilled", stream.read<uint32_t>());
            data.setv("wardsPlaced", stream.read<uint32_t>());
            stream.ignore(2 * 0x4);
            stream.ignore(0x2); // Padding

            return Value::create(data);
        }
    };
}

#endif /* defined(__libol__PacketDecoders__) */