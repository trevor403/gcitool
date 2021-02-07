#include <vector>
#include <fstream>
#include <iostream>

#include "popl.hpp"
// #include "picojson.h"
#include <boost/algorithm/string/join.hpp>

#include <LibPkmGC/Colosseum/SaveEditing/Save.h>
#include <LibPkmGC/XD/SaveEditing/Save.h>

// #include <LibPkmGC/GC/Common/Pokemon.h>

using namespace LibPkmGC;

const size_t GCN_COLOSSEUM_GCI_SIZE = 0x60040;
const size_t GCN_COLOSSEUM_SLOT_SIZE = 0x1e000;

const size_t GCN_XD_GCI_SIZE = 0x56040;
const size_t GCN_XD_SLOT_SIZE = 0x28000;

const int COLOSSEUM_ID = 19;
const int XD_ID        = 20;

const int EXTRACT = 0;
const int REPLACE = 1;

//// modes
// list_gci_slots (input_gci)
// extract_gci_slot (input_gci, index, output_slot)
// info_gci_slot (input_gci, index)
// replace_all_slots (input_slot, output_gci)
// replace_first_valid_slot (input_slot, output_gci)
// replace_most_recent_slot (input_slot, output_gci)
// copy_battle_data_into_pc (input_gci)
// inject_pc_into_slot (input_slot, pokemon_json, output_slot)

using namespace popl;

void mode_error() {
    const static std::vector<std::string> modes {
        "list_gci_slots",
        "info_gci_slot",
        "info_raw_slot",
        "replace_all_slots",
        "copy_battle_data_into_pc",
    };

    std::cerr << "Must provide an action mode [" << boost::join(modes, ", ") << "]" << std::endl;
    exit(1);
}

void mode_option_error(OptionParser op) {
    std::cout << op << std::endl;
    exit(1);
}

constexpr unsigned int hash(const char *s, int off = 0) {                        
    return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];                           
}

void info_gci_slot(std::string input_gci, uint32_t index) {
    std::fstream save_file_stream(input_gci, std::ios::in | std::ios::out | std::ios::binary);
    std::vector<uint8_t> save_file_buf((std::istreambuf_iterator<char>(save_file_stream)), std::istreambuf_iterator<char>());
    size_t save_size = save_file_buf.size();

	GC::SaveEditing::Save* save;
    int game_save_type = 0;
	if (save_size == GCN_COLOSSEUM_GCI_SIZE) {
        game_save_type = COLOSSEUM_ID;
		save = new Colosseum::SaveEditing::Save(save_file_buf.data(), true);
	} else if (save_size == GCN_XD_GCI_SIZE) {
        game_save_type = XD_ID;
		save = new XD::SaveEditing::Save(save_file_buf.data(), true);
	}
    GC::SaveEditing::SaveSlot* save_slot = save->saveSlots[index];
    size_t save_slot_size = save_slot->getSize();

    const char *game_name = game_save_type == XD_ID ? "XD" : "Col";
    const char *status = save_slot->isCorrupt() ? "corrupt" : "valid";
    printf("%s gci save_size=%x slot_size=%x (%s)\n", game_name, save_size, save_slot_size, status);

    std::cout << "Trainer name: " << save_slot->player->trainer->trainerName->toUTF8() << std::endl;

    std::cout << "Party Pokemon dump:" << std::endl;
    for (size_t i = 0; i < 6; ++i) {
        GC::Pokemon* pkm = save_slot->player->trainer->party[i];
        printf("Party pokemon %s\n", pkm->name->toUTF8());
    }

    std::cout << "PC Pokemon dump:" << std::endl;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 30; ++j) {
            GC::Pokemon* pkm = (GC::Pokemon*)save_slot->PC->boxes[i]->pkm[j];
            printf("Slot %d pokemon %s\n", i, pkm->name->toUTF8());
        }
    }

    return;
}

void list_gci_slots(std::string input_gci) {
    std::fstream save_file_stream(input_gci, std::ios::in | std::ios::out | std::ios::binary);
    std::vector<uint8_t> save_file_buf((std::istreambuf_iterator<char>(save_file_stream)), std::istreambuf_iterator<char>());
    size_t save_size = save_file_buf.size();

	GC::SaveEditing::Save* save;
    int game_save_type = 0;
	if (save_size == GCN_COLOSSEUM_GCI_SIZE) {
        game_save_type = COLOSSEUM_ID;
		save = new Colosseum::SaveEditing::Save(save_file_buf.data(), true);
	} else if (save_size == GCN_XD_GCI_SIZE) {
        game_save_type = XD_ID;
		save = new XD::SaveEditing::Save(save_file_buf.data(), true);
	}

    for (int i = 0; i < save->nbSlots; i++) {
        GC::SaveEditing::SaveSlot* save_slot = save->saveSlots[i];
        std::string name = save_slot->player->trainer->trainerName->toUTF8();
        std::cout << "Trainer name: " << name << " - " << "Saves: " << save_slot->saveCount << std::endl;
    }

    return;
}

void replace_all_slots(std::string input_slot, std::string output_gci) {
    std::fstream save_file_stream(output_gci, std::ios::in | std::ios::out | std::ios::binary);
    std::vector<uint8_t> save_file_buf((std::istreambuf_iterator<char>(save_file_stream)), std::istreambuf_iterator<char>());
    size_t save_size = save_file_buf.size();

    std::ifstream save_slot_stream(input_slot, std::ios::in | std::ios::binary);
    std::vector<uint8_t> save_slot_buf((std::istreambuf_iterator<char>(save_slot_stream)), std::istreambuf_iterator<char>());

	GC::SaveEditing::Save* save;
    int game_save_type = 0;
	if (save_size == GCN_COLOSSEUM_GCI_SIZE) {
        game_save_type = COLOSSEUM_ID;
		save = new Colosseum::SaveEditing::Save(save_file_buf.data(), true);
	} else if (save_size == GCN_XD_GCI_SIZE) {
        game_save_type = XD_ID;
		save = new XD::SaveEditing::Save(save_file_buf.data(), true);
	}

    for (int i = 0; i < save->nbSlots; i++) {
        GC::SaveEditing::SaveSlot* save_slot = save->saveSlots[i];
        save_slot->reload(save_slot_buf.data(), 1); // is decrypted
        save_slot->saveCount = i;
        save_slot->checkBothChecksums(true, true);

        bool globalChecksum;
        bool headerChecksum;
        std::tie(globalChecksum, headerChecksum) = save_slot->checkBothChecksums(true, true);;

        std::cout << "Global checksum: " << globalChecksum << std::endl;
        std::cout << "Header checksum: " << globalChecksum << std::endl;
    }

    save->saveEncrypted(save_file_buf.data(), true, true);

    save_file_stream.seekp(0, std::ios::beg);
    save_file_stream.write((const char*)save_file_buf.data(), save_size);
    save_file_stream.close();

}

void info_raw_slot(std::string input_slot, std::string game_name) {
    std::ifstream save_slot_stream(input_slot, std::ios::in | std::ios::binary);
    std::vector<uint8_t> save_slot_buf((std::istreambuf_iterator<char>(save_slot_stream)), std::istreambuf_iterator<char>());

	GC::SaveEditing::SaveSlot* save_slot;
	if (game_name == "Col") {
		save_slot = new Colosseum::SaveEditing::SaveSlot;
	} else if (game_name == "XD") {
		save_slot = new XD::SaveEditing::SaveSlot;
	} else {
        std::cerr << "Unknown game " << game_name << " please use XD or Col" << std::endl;
        exit(1);
    }
    save_slot->reload(save_slot_buf.data(), 1); // is decrypted

    const char *status = save_slot->isCorrupt() ? "corrupt" : "valid";
    printf("%s raw_slot slot_size=%x (%s)\n", game_name.c_str(), save_slot_buf.size(), status);

    std::cout << "Trainer name: " << save_slot->player->trainer->trainerName->toUTF8() << std::endl;

    std::cout << "Party Pokemon dump:" << std::endl;
    for (size_t i = 0; i < 6; ++i) {
        GC::Pokemon* pkm = save_slot->player->trainer->party[i];
        printf("Party pokemon %s\n", pkm->name->toUTF8());
    }

    std::cout << "PC Pokemon dump:" << std::endl;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 30; ++j) {
            GC::Pokemon* pkm = (GC::Pokemon*)save_slot->PC->boxes[i]->pkm[j];
            printf("Slot %d pokemon %s\n", i, pkm->name->toUTF8());
        }
    }

    //// Col only

    if (game_name != "Col") {
        return;
    }

    const u32 offsetToParty = 0x80;

    u8 *battleModePokemon = save_slot->battleMode->data + offsetToParty;
    for (size_t i = 0; i < 6; i++) {
        Colosseum::Pokemon* pkm = new Colosseum::Pokemon(battleModePokemon);
        printf("BattleMode %d pokemon %s\n", i, pkm->name->toUTF8());
        battleModePokemon += pkm->fixedSize;
    }

    return;
    
}

void copy_battle_data_into_pc(std::string gci_path) {
    std::fstream save_file_stream(gci_path, std::ios::in | std::ios::out | std::ios::binary);
    std::vector<uint8_t> save_file_buf((std::istreambuf_iterator<char>(save_file_stream)), std::istreambuf_iterator<char>());
    size_t save_size = save_file_buf.size();

	GC::SaveEditing::Save* save;
    int game_save_type = 0;
	if (save_size == GCN_COLOSSEUM_GCI_SIZE) {
        game_save_type = COLOSSEUM_ID;
		save = new Colosseum::SaveEditing::Save(save_file_buf.data(), true);
	} else if (save_size == GCN_XD_GCI_SIZE) {
        game_save_type = XD_ID;
		save = new XD::SaveEditing::Save(save_file_buf.data(), true);
	}
    GC::SaveEditing::SaveSlot* save_slot = save->getMostRecentSlot(0);
    size_t save_slot_size = save_slot->getSize();

    const char *game_name = game_save_type == XD_ID ? "XD" : "Col";
    printf("%s gci save_size=%x slot_size=%x\n", game_name, save_size, save_slot_size);

    const u32 offsetToParty = 0x80;

    u8 *battleModePokemon = save_slot->battleMode->data + offsetToParty;
    for (size_t i = 0; i < 6; i++) {
        Colosseum::Pokemon* pkm = new Colosseum::Pokemon(battleModePokemon);
        printf("BattleMode %d pokemon %s\n", i, pkm->name->toUTF8());

        u32 box_index = 30 - 6 + i;
        save_slot->PC->boxes[2]->pkm[box_index] = pkm;
        battleModePokemon += pkm->fixedSize;
    }


    save_slot->checkBothChecksums(true, true);
    save->saveEncrypted(save_file_buf.data(), true, true);

    save_file_stream.seekp(0, std::ios::beg);
    save_file_stream.write((const char*)save_file_buf.data(), save_size);
    save_file_stream.close();

    const char *status = save_slot->isCorrupt() ? "corrupt" : "valid";
    printf("replaced %s save slot in %s\n", status, gci_path.c_str());

    return;   
}

int main(int argc, char *argv[]) {
    int game_save_type = 0;
    int edit_mode = 0;

    OptionParser op("Mode Options");
	op.parse(argc, argv);

    auto actions = op.non_option_args();
    if (actions.empty()) {
        mode_error();
    }

    switch(hash(actions.at(0).c_str())) {
    case hash("list_gci_slots"): {
        auto input_gci_option = op.add<Value<std::string>, Attribute::required>("", "input_gci", "input save file");
        try { op.parse(argc, argv); } catch (...) { mode_option_error(op); }

        auto input_gci = input_gci_option->value();
        list_gci_slots(input_gci);
        } break;
// case hash("extract_gci_slot"): {
//     // extract_gci_slot(input_gci, index, output_slot)
// } break;
    case hash("info_gci_slot"): {
        auto input_gci_option = op.add<Value<std::string>, Attribute::required>("", "input_gci", "input save file");
        auto index_option = op.add<Value<uint32_t>>("i", "index", "slot index", 0);
        try { op.parse(argc, argv); } catch (...) { mode_option_error(op); }

        auto input_gci = input_gci_option->value();
        auto index = index_option->value();

        info_gci_slot(input_gci, index);
        } break;
    case hash("info_raw_slot"): {
        auto input_slot_option = op.add<Value<std::string>, Attribute::required>("", "input_slot", "input save slot file");
        auto game_option = op.add<Value<std::string>, Attribute::required>("", "game", "game name XD or Col");
        try { op.parse(argc, argv); } catch (...) { mode_option_error(op); }

        auto input_slot = input_slot_option->value();
        auto game_name = game_option->value();

        info_raw_slot(input_slot, game_name);
        } break;
    case hash("replace_all_slots"): {
        auto input_slot_option = op.add<Value<std::string>, Attribute::required>("", "input_slot", "input save slot file");
        auto output_gci_option = op.add<Value<std::string>, Attribute::required>("", "output_gci", "output save file");
        try { op.parse(argc, argv); } catch (...) { mode_option_error(op); }
        
        auto input_slot = input_slot_option->value();
        auto output_gci = output_gci_option->value();

        replace_all_slots(input_slot, output_gci);
        } break;
// case hash("replace_first_valid_slot"): {
//     // replace_first_valid_slot(input_slot, output_gci)
//     } break;
// case hash("replace_most_recent_slot"): {
//     // replace_most_recent_slot(input_slot, output_gci)
//     } break;
    case hash("copy_battle_data_into_pc"): {
        auto gci_option = op.add<Value<std::string>, Attribute::required>("", "gci", "save file");
        try { op.parse(argc, argv); } catch (...) { mode_option_error(op); }

        auto gci_path = gci_option->value();

        copy_battle_data_into_pc(gci_path);
        } break;
// case hash("inject_pc_into_slot"): {
//     // inject_pc_into_slot(input_slot, pokemon_json, output_slot)
//     } break;
    default:
        mode_error();
        break;
    }

    //     std::ofstream save_slot_stream(save_slot_path, std::ofstream::binary);
    //     save_slot_stream.write((const char*)save_slot->data, save_slot_size);
    //     save_slot_stream.close();

    //     const char *status = save_slot->isCorrupt() ? "corrupt" : "valid";
    //     printf("extracted %s save slot to %s\n", status, save_slot_path.c_str());

    return 0;
}
