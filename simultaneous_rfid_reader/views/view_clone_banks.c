#include "view_clone_banks.h"
#include "../helpers/saved_epc_functions.h"

static const char* const CLONE_ON_OFF[] = {"OFF", "ON"};

// ─── helpers ─────────────────────────────────────────────────────────────────

static void clone_update_start_item(UHFReaderApp* App) {
    variable_item_set_current_value_text(
        App->CloneStartItem, App->CloneMask ? "Clone ->" : "---");
}

// Build a deep copy of the source tag into App->CloneSourceTag.
// For ActionFromLive the data comes from the ViewEpc model (hex strings).
// For ActionFromSaved it comes from Saved_EPCs.txt at SelectedTagIndex.
static void build_clone_source_tag(UHFReaderApp* App) {
    uhf_tag_reset(App->CloneSourceTag);

    if(App->ActionContext == ActionFromLive) {
        FuriString* Epc = furi_string_alloc();
        FuriString* Tid = furi_string_alloc();
        FuriString* Res = furi_string_alloc();
        FuriString* Mem = furi_string_alloc();
        FuriString* Pc  = furi_string_alloc();
        FuriString* Crc = furi_string_alloc();

        with_view_model(
            App->ViewEpc,
            UHFRFIDTagModel * Live,
            {
                furi_string_set(Epc, Live->Epc);
                furi_string_set(Tid, Live->Tid);
                furi_string_set(Res, Live->Reserved);
                furi_string_set(Mem, Live->User);
                furi_string_set(Pc,  Live->Pc);
                furi_string_set(Crc, Live->Crc);
            },
            false);

        // EPC
        uint8_t epc_bytes[EPC_MAX_BANK_SIZE] = {0};
        size_t  epc_len = 0;
        hex_string_to_bytes(furi_string_get_cstr(Epc), epc_bytes, &epc_len);
        if(epc_len > 0) uhf_tag_set_epc(App->CloneSourceTag, epc_bytes, epc_len);

        uint16_t pc_arr[4] = {0};
        size_t   pc_len = 0;
        hex_string_to_uint16(furi_string_get_cstr(Pc), pc_arr, &pc_len);
        if(pc_len > 0) uhf_tag_set_epc_pc(App->CloneSourceTag, pc_arr[0]);

        uint16_t crc_arr[4] = {0};
        size_t   crc_len = 0;
        hex_string_to_uint16(furi_string_get_cstr(Crc), crc_arr, &crc_len);
        if(crc_len > 0) uhf_tag_set_epc_crc(App->CloneSourceTag, crc_arr[0]);

        // TID
        uint8_t tid_bytes[TID_MAX_BANK_SIZE] = {0};
        size_t  tid_len = 0;
        hex_string_to_bytes(furi_string_get_cstr(Tid), tid_bytes, &tid_len);
        if(tid_len > 0) uhf_tag_set_tid(App->CloneSourceTag, tid_bytes, tid_len);

        // User
        uint8_t user_bytes[USER_MAX_BANK_SIZE] = {0};
        size_t  user_len = 0;
        hex_string_to_bytes(furi_string_get_cstr(Mem), user_bytes, &user_len);
        if(user_len > 0) uhf_tag_set_user(App->CloneSourceTag, user_bytes, user_len);

        // Reserved (first 4 bytes = kill pwd, next 4 = access pwd)
        uint8_t res_bytes[RESERVED_MAX_BANK_SIZE] = {0};
        size_t  res_len = 0;
        hex_string_to_bytes(furi_string_get_cstr(Res), res_bytes, &res_len);
        if(res_len >= 4) uhf_tag_set_kill_pwd(App->CloneSourceTag, res_bytes, 4);
        if(res_len >= 8) uhf_tag_set_access_pwd(App->CloneSourceTag, res_bytes + 4, 4);
        App->CloneSourceTag->reserved->size = res_len;

        furi_string_free(Epc);
        furi_string_free(Tid);
        furi_string_free(Res);
        furi_string_free(Mem);
        furi_string_free(Pc);
        furi_string_free(Crc);

    } else {
        // ActionFromSaved — read from Saved_EPCs.txt
        FuriString* TempStr = furi_string_alloc();
        FuriString* TempTag = furi_string_alloc();

        if(flipper_format_file_open_existing(App->EpcFile, APP_DATA_PATH("Saved_EPCs.txt"))) {
            furi_string_printf(TempStr, "Tag%ld", App->SelectedTagIndex);
            if(flipper_format_read_string(
                   App->EpcFile, furi_string_get_cstr(TempStr), TempTag)) {
                const char* s = furi_string_get_cstr(TempTag);

                char* epc_str = extract_epc(s);
                char* tid_str = extract_tid(s);
                char* res_str = extract_res(s);
                char* mem_str = extract_mem(s);
                char* pc_str  = extract_pc(s);
                char* crc_str = extract_crc(s);

                if(epc_str) {
                    uint8_t b[EPC_MAX_BANK_SIZE] = {0};
                    size_t l = 0;
                    hex_string_to_bytes(epc_str, b, &l);
                    if(l > 0) uhf_tag_set_epc(App->CloneSourceTag, b, l);
                    free(epc_str);
                }
                if(pc_str) {
                    uint16_t arr[4] = {0};
                    size_t l = 0;
                    hex_string_to_uint16(pc_str, arr, &l);
                    if(l > 0) uhf_tag_set_epc_pc(App->CloneSourceTag, arr[0]);
                    free(pc_str);
                }
                if(crc_str) {
                    uint16_t arr[4] = {0};
                    size_t l = 0;
                    hex_string_to_uint16(crc_str, arr, &l);
                    if(l > 0) uhf_tag_set_epc_crc(App->CloneSourceTag, arr[0]);
                    free(crc_str);
                }
                if(tid_str) {
                    uint8_t b[TID_MAX_BANK_SIZE] = {0};
                    size_t l = 0;
                    hex_string_to_bytes(tid_str, b, &l);
                    if(l > 0) uhf_tag_set_tid(App->CloneSourceTag, b, l);
                    free(tid_str);
                }
                if(mem_str) {
                    uint8_t b[USER_MAX_BANK_SIZE] = {0};
                    size_t l = 0;
                    hex_string_to_bytes(mem_str, b, &l);
                    if(l > 0) uhf_tag_set_user(App->CloneSourceTag, b, l);
                    free(mem_str);
                }
                if(res_str) {
                    uint8_t b[RESERVED_MAX_BANK_SIZE] = {0};
                    size_t l = 0;
                    hex_string_to_bytes(res_str, b, &l);
                    if(l >= 4) uhf_tag_set_kill_pwd(App->CloneSourceTag, b, 4);
                    if(l >= 8) uhf_tag_set_access_pwd(App->CloneSourceTag, b + 4, 4);
                    App->CloneSourceTag->reserved->size = l;
                    free(res_str);
                }
            }
            flipper_format_file_close(App->EpcFile);
        }

        furi_string_free(TempStr);
        furi_string_free(TempTag);
    }
}

// ─── VariableItem change callbacks ───────────────────────────────────────────

static void clone_epc_changed(VariableItem* item) {
    UHFReaderApp* App = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    // Reject ON if no EPC data
    if(idx == 1 && App->CloneSourceTag->epc->size == 0) {
        variable_item_set_current_value_index(item, 0);
        variable_item_set_current_value_text(item, "N/A");
        return;
    }
    variable_item_set_current_value_text(item, CLONE_ON_OFF[idx]);
    if(idx) App->CloneMask |= WRITE_EPC;
    else     App->CloneMask &= ~(uint16_t)WRITE_EPC;
    clone_update_start_item(App);
}

static void clone_tid_changed(VariableItem* item) {
    UHFReaderApp* App = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    if(idx == 1 && App->CloneSourceTag->tid->size == 0) {
        variable_item_set_current_value_index(item, 0);
        variable_item_set_current_value_text(item, "N/A");
        return;
    }
    variable_item_set_current_value_text(item, CLONE_ON_OFF[idx]);
    if(idx) App->CloneMask |= WRITE_TID;
    else     App->CloneMask &= ~(uint16_t)WRITE_TID;
    clone_update_start_item(App);
}

static void clone_user_changed(VariableItem* item) {
    UHFReaderApp* App = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    if(idx == 1 && App->CloneSourceTag->user->size == 0) {
        variable_item_set_current_value_index(item, 0);
        variable_item_set_current_value_text(item, "N/A");
        return;
    }
    variable_item_set_current_value_text(item, CLONE_ON_OFF[idx]);
    if(idx) App->CloneMask |= WRITE_USER;
    else     App->CloneMask &= ~(uint16_t)WRITE_USER;
    clone_update_start_item(App);
}

static void clone_res_changed(VariableItem* item) {
    UHFReaderApp* App = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    if(idx == 1 && App->CloneSourceTag->reserved->size < 4) {
        variable_item_set_current_value_index(item, 0);
        variable_item_set_current_value_text(item, "N/A");
        return;
    }
    variable_item_set_current_value_text(item, CLONE_ON_OFF[idx]);
    if(idx) App->CloneMask |= WRITE_RFU;
    else     App->CloneMask &= ~(uint16_t)WRITE_RFU;
    clone_update_start_item(App);
}

// ─── Enter callback (item selection) ─────────────────────────────────────────

static void clone_banks_item_enter(void* context, uint32_t index) {
    UHFReaderApp* App = context;
    if(index != CLONE_BANKS_IDX_START) return;
    if(!App->CloneMask) return;
    view_dispatcher_switch_to_view(App->ViewDispatcher, UHFReaderViewClone);
}

// ─── Navigation ──────────────────────────────────────────────────────────────

uint32_t uhf_reader_navigation_clone_banks_callback(void* context) {
    UNUSED(context);
    return UHFReaderViewTagAction;
}

// ─── View enter callback ──────────────────────────────────────────────────────

void view_clone_banks_enter_callback(void* context) {
    UHFReaderApp* App = context;
    // Deep-copy the source tag at entry time (safe snapshot, worker can't mutate it)
    build_clone_source_tag(App);

    // Reset selection mask
    App->CloneMask = 0;

    bool epc_avail  = (App->CloneSourceTag->epc->size > 0);
    bool tid_avail  = (App->CloneSourceTag->tid->size > 0);
    bool user_avail = (App->CloneSourceTag->user->size > 0);
    bool res_avail  = (App->CloneSourceTag->reserved->size >= 4);

    // EPC: default ON when data is present
    variable_item_set_current_value_index(App->CloneEpcItem, epc_avail ? 1 : 0);
    variable_item_set_current_value_text(App->CloneEpcItem, epc_avail ? "ON" : "N/A");
    if(epc_avail) App->CloneMask |= WRITE_EPC;

    // TID: always default OFF (rewritable TID tags are rare)
    variable_item_set_current_value_index(App->CloneTidItem, 0);
    variable_item_set_current_value_text(App->CloneTidItem, tid_avail ? "OFF" : "N/A");

    // User: default OFF
    variable_item_set_current_value_index(App->CloneUserItem, 0);
    variable_item_set_current_value_text(App->CloneUserItem, user_avail ? "OFF" : "N/A");

    // Reserved: default OFF
    variable_item_set_current_value_index(App->CloneResItem, 0);
    variable_item_set_current_value_text(App->CloneResItem, res_avail ? "OFF" : "N/A");

    clone_update_start_item(App);
}

// ─── Alloc / Free ─────────────────────────────────────────────────────────────

void view_clone_banks_alloc(UHFReaderApp* App) {
    App->CloneSourceTag = uhf_tag_alloc();
    App->CloneMask = 0;

    App->VariableItemListClone = variable_item_list_alloc();

    App->CloneEpcItem = variable_item_list_add(
        App->VariableItemListClone, "EPC", 2, clone_epc_changed, App);

    App->CloneTidItem = variable_item_list_add(
        App->VariableItemListClone, "TID (RW tag only)", 2, clone_tid_changed, App);

    App->CloneUserItem = variable_item_list_add(
        App->VariableItemListClone, "User", 2, clone_user_changed, App);

    // Reserved bank is dangerous — label warns the user
    App->CloneResItem = variable_item_list_add(
        App->VariableItemListClone, "Reserved [!DANGER]", 2, clone_res_changed, App);

    // "Start Clone ->" action item: 1 value so it cannot be cycled
    App->CloneStartItem = variable_item_list_add(
        App->VariableItemListClone, "Start Clone", 1, NULL, App);
    variable_item_set_current_value_text(App->CloneStartItem, "---");

    variable_item_list_set_enter_callback(
        App->VariableItemListClone, clone_banks_item_enter, App);

    View* vil_view = variable_item_list_get_view(App->VariableItemListClone);
    view_set_previous_callback(vil_view, uhf_reader_navigation_clone_banks_callback);

    view_dispatcher_add_view(
        App->ViewDispatcher,
        UHFReaderViewCloneBanks,
        vil_view);
}

void view_clone_banks_free(UHFReaderApp* App) {
    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewCloneBanks);
    variable_item_list_free(App->VariableItemListClone);
    uhf_tag_free(App->CloneSourceTag);
    App->CloneSourceTag = NULL;
}
