#include <vector>
#include <fstream>
#include <iostream>

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

int main(int argc, char *argv[]) {
    int game_save_type = 0;
    int edit_mode = 0;

    if (argc < 4) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "\t" << argv[0] << " [edit mode] [path to .gci] [path to raw save slot]" << std::endl;
        std::cerr << "\nEdit mode must be either [extract] or [replace]" << std::endl;
        exit(1);
    }

    if (argv[1] == std::string("extract")) {
        edit_mode = EXTRACT;
    } else if (argv[1] == std::string("replace")) {
        edit_mode = REPLACE;
    } else {
        std::cerr << "Edit mode [" << argv[1] << "] is invalid, please use [extract] or [replace]" << std::endl;
        exit(1);
    }

    std::string save_file_path(argv[2]);
    std::fstream save_file_stream(save_file_path, std::ios::in | std::ios::out | std::ios::binary);
    std::vector<uint8_t> save_file_buf((std::istreambuf_iterator<char>(save_file_stream)), std::istreambuf_iterator<char>());
    size_t save_size = save_file_buf.size();

	GC::SaveEditing::Save* save;
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

    std::string save_slot_path(argv[3]);
    if (edit_mode == EXTRACT) {
        std::ofstream save_slot_stream(save_slot_path, std::ofstream::binary);
        save_slot_stream.write((const char*)save_slot->data, save_slot_size);
        save_slot_stream.close();

        const char *status = save_slot->isCorrupt() ? "corrupt" : "valid";
        printf("extracted %s save slot to %s\n", status, save_slot_path.c_str());
    }

    if (edit_mode == REPLACE) {
        std::ifstream save_slot_stream(save_file_path, std::ios::in | std::ios::binary);
        std::vector<uint8_t> save_slot_buf((std::istreambuf_iterator<char>(save_file_stream)), std::istreambuf_iterator<char>());
        save_slot->reload(save_slot_buf.data(), 1); // is decrypted
        // save_slot->player->trainer->trainerName = new GC::PokemonString("TEST");
        save_slot->checkBothChecksums(true, true);
        save->saveEncrypted(save_file_buf.data(), true, true);

        save_file_stream.seekp(0, std::ios::beg);
        save_file_stream.write((const char*)save_file_buf.data(), save_size);
        save_file_stream.close();

        const char *status = save_slot->isCorrupt() ? "corrupt" : "valid";
        printf("replaced %s save slot in %s\n", status, save_file_path.c_str());
    }

    return 0;
}