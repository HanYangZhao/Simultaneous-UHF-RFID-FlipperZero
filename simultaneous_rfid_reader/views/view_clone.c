#include "view_clone.h"
#include "../helpers/saved_epc_functions.h"
#include "view_read.h"

// ─── helpers ──────────────────────────────────────────────────────────────────

// Build a human-readable bank summary from a WriteMask bitmask.
// dst must be at least 28 bytes.
static void clone_mask_to_str(uint16_t mask, char* dst, size_t dst_size) {
    const char* epc  = (mask & WRITE_EPC)  ? "EPC "  : "";
    const char* tid  = (mask & WRITE_TID)  ? "TID "  : "";
    const char* user = (mask & WRITE_USER) ? "User " : "";
    const char* rsvd = (mask & WRITE_RFU)  ? "Rsvd"  : "";
    snprintf(dst, dst_size, "%s%s%s%s", epc, tid, user, rsvd);
    // Trim trailing space
    size_t len = strlen(dst);
    if(len > 0 && dst[len - 1] == ' ') dst[len - 1] = '\0';
    if(dst[0] == '\0') snprintf(dst, dst_size, "None");
}

// Stop any in-flight worker and (re)start the clone scan worker WITHOUT
// changing the view phase. Safe to call from the view-dispatcher (UI) thread.
static void start_clone_scan_worker(UHFReaderApp* App) {
    uhf_worker_stop(App->YRM100XWorker);
    uhf_worker_start(
        App->YRM100XWorker,
        UHFWorkerStateCloneScan,
        uhf_clone_worker_callback,
        App);
}

// Start (or restart) the clone scan worker and show the scanning screen.
static void start_clone_scan(UHFReaderApp* App) {
    bool redraw = true;
    with_view_model(
        App->ViewClone,
        UHFReaderCloneModel * Model,
        { Model->phase = ClonePhaseScanning; },
        redraw);

    start_clone_scan_worker(App);
}

// ─── Worker callback ──────────────────────────────────────────────────────────

void uhf_clone_worker_callback(UHFWorkerEvent event, void* context) {
    UHFReaderApp* App = context;
    if(event == UHFWorkerEventCardDetected) {
        view_dispatcher_send_custom_event(App->ViewDispatcher, UHFCustomEventCloneScanDone);
    } else if(event == UHFWorkerEventNoTagDetected) {
        notification_message(App->Notifications, &uhf_sequence_blink_stop);
        view_dispatcher_send_custom_event(App->ViewDispatcher, UHFCustomEventCloneScanTimeout);
    } else if(event == UHFWorkerEventSuccess) {
        notification_message(App->Notifications, &uhf_sequence_blink_stop);
        notification_message(App->Notifications, &sequence_success);
        view_dispatcher_send_custom_event(App->ViewDispatcher, UHFCustomEventCloneWriteDone);
    } else if(event == UHFWorkerEventAccessDenied) {
        notification_message(App->Notifications, &uhf_sequence_blink_stop);
        notification_message(App->Notifications, &sequence_error);
        view_dispatcher_send_custom_event(App->ViewDispatcher, UHFCustomEventCloneAccessDenied);
    } else if(event == UHFWorkerEventFail) {
        notification_message(App->Notifications, &uhf_sequence_blink_stop);
        notification_message(App->Notifications, &sequence_error);
        view_dispatcher_send_custom_event(App->ViewDispatcher, UHFCustomEventCloneWriteFail);
    }
    // UHFWorkerEventAborted: user pressed Back — no UI update needed
}

// ─── Draw callback ────────────────────────────────────────────────────────────

void uhf_reader_view_clone_draw_callback(Canvas* canvas, void* model) {
    UHFReaderCloneModel* Model = model;
    canvas_clear(canvas);

    switch(Model->phase) {
    case ClonePhaseScanning:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "Clone Tag");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 28, AlignCenter, AlignTop, "Place target tag...");
        canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignTop, "Scanning...");
        canvas_draw_str_aligned(canvas, 64, 56, AlignCenter, AlignTop, "[Back] Cancel");
        break;

    case ClonePhaseConfirm: {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 2, AlignCenter, AlignTop, "Found Tag");
        canvas_set_font(canvas, FontSecondary);

        // EPC — truncate to 20 chars + "..." if longer
        char epc_display[24];
        size_t epc_len = strlen(Model->target_epc_str);
        if(epc_len > 20) {
            strncpy(epc_display, Model->target_epc_str, 20);
            epc_display[20] = '.';
            epc_display[21] = '.';
            epc_display[22] = '.';
            epc_display[23] = '\0';
        } else {
            strncpy(epc_display, Model->target_epc_str, sizeof(epc_display));
            epc_display[sizeof(epc_display) - 1] = '\0';
        }
        canvas_draw_str_aligned(canvas, 64, 16, AlignCenter, AlignTop, epc_display);

        // Bank summary
        char bank_str[28] = "Write: ";
        clone_mask_to_str(Model->clone_mask, bank_str + 7, sizeof(bank_str) - 7);
        canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignTop, bank_str);

        // Hints
        elements_button_center(canvas, "Write");
        break;
    }

    case ClonePhaseWriting:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "Clone Tag");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignTop, "Writing...");
        break;

    case ClonePhaseSuccess: {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "Cloned!");
        canvas_set_font(canvas, FontSecondary);
        char count_str[20];
        snprintf(count_str, sizeof(count_str), "Total: %u", (unsigned)Model->clone_count);
        canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignTop, count_str);
        elements_button_center(canvas, "Scan Next");
        break;
    }

    case ClonePhaseTimeout:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "No Tag Found");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 28, AlignCenter, AlignTop, "Timed out after 10s.");
        elements_button_center(canvas, "Scan Next");
        break;

    case ClonePhaseFailed:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "Write Failed");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 28, AlignCenter, AlignTop, "Tag did not respond.");
        elements_button_center(canvas, "Retry");
        break;

    case ClonePhaseAccessDenied:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignTop, "Tag Locked");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 28, AlignCenter, AlignTop, "Wrong access password.");
        elements_button_center(canvas, "Retry");
        break;
    }
}

// ─── Input callback ───────────────────────────────────────────────────────────

bool uhf_reader_view_clone_input_callback(InputEvent* event, void* context) {
    UHFReaderApp* App = context;
    if(event->type != InputTypeShort) return false;

    ClonePhase phase;
    with_view_model(App->ViewClone, UHFReaderCloneModel * M, { phase = M->phase; }, false);

    if(event->key == InputKeyOk) {
        if(phase == ClonePhaseConfirm) {
            // Populate worker source (NewTag) from the deep-copied CloneSourceTag
            UHFTag* src  = App->CloneSourceTag;
            UHFTag* dest = App->YRM100XWorker->NewTag;
            uhf_tag_reset(dest);
            if(src->epc->size > 0) {
                uhf_tag_set_epc(dest, src->epc->data, src->epc->size);
                uhf_tag_set_epc_pc(dest, src->epc->pc);
                uhf_tag_set_epc_crc(dest, src->epc->crc);
            }
            if(src->tid->size > 0)
                uhf_tag_set_tid(dest, src->tid->data, src->tid->size);
            if(src->user->size > 0)
                uhf_tag_set_user(dest, src->user->data, src->user->size);
            if(src->reserved->size > 0) {
                uhf_tag_set_kill_pwd(dest, src->reserved->kill_password, 4);
                uhf_tag_set_access_pwd(dest, src->reserved->access_password, 4);
                dest->reserved->size = src->reserved->size;
            }

            App->YRM100XWorker->CloneMask   = App->CloneMask;
            App->YRM100XWorker->DefaultAP   = 0; // attempt with zero access pwd
            App->YRM100XWorker->Targeted    = false;

            bool redraw = true;
            with_view_model(
                App->ViewClone,
                UHFReaderCloneModel * Model,
                { Model->phase = ClonePhaseWriting; },
                redraw);

            notification_message(App->Notifications, &uhf_sequence_blink_start_cyan);
            uhf_worker_start(
                App->YRM100XWorker,
                UHFWorkerStateCloneWrite,
                uhf_clone_worker_callback,
                App);
            return true;
        }
        if(phase == ClonePhaseFailed || phase == ClonePhaseAccessDenied) {
            // Retry: go back to scanning
            start_clone_scan(App);
            return true;
        }
        if(phase == ClonePhaseSuccess || phase == ClonePhaseTimeout) {
            // Scan next: restart the scan loop for another target
            notification_message(App->Notifications, &uhf_sequence_blink_start_cyan);
            start_clone_scan(App);
            return true;
        }
    }
    return false;
}

// ─── Custom event callback ────────────────────────────────────────────────────

bool uhf_reader_view_clone_custom_event_callback(uint32_t event, void* context) {
    UHFReaderApp* App = context;

    if(event == UHFCustomEventCloneScanDone) {
        // A tag was found by the scan worker. Capture its EPC for display.
        UHFTag* found = App->YRM100XWorker->uhf_tag_wrapper->uhf_tag;
        char epc_hex[65] = {0};
        if(found && found->epc->size > 0) {
            char* hex = convert_to_hex_string(found->epc->data, found->epc->size);
            if(hex) {
                strncpy(epc_hex, hex, sizeof(epc_hex) - 1);
                free(hex);
            }
        } else {
            strncpy(epc_hex, "????????", sizeof(epc_hex) - 1);
        }

        bool redraw = true;
        with_view_model(
            App->ViewClone,
            UHFReaderCloneModel * Model,
            {
                Model->phase = ClonePhaseConfirm;
                strncpy(Model->target_epc_str, epc_hex, sizeof(Model->target_epc_str) - 1);
                Model->clone_mask = App->CloneMask;
            },
            redraw);
        return true;
    }

    if(event == UHFCustomEventCloneWriteDone) {
        dolphin_deed(DolphinDeedNfcReadSuccess);
        bool redraw = true;
        with_view_model(
            App->ViewClone,
            UHFReaderCloneModel * Model,
            {
                Model->clone_count++;
                Model->phase = ClonePhaseSuccess;
            },
            redraw);
        // Stay on the Success screen and wait for the user: [OK] scans the next
        // tag, [Back] exits. We do NOT auto-restart the scan loop.
        return true;
    }

    if(event == UHFCustomEventCloneScanTimeout) {
        bool redraw = true;
        with_view_model(
            App->ViewClone,
            UHFReaderCloneModel * Model,
            { Model->phase = ClonePhaseTimeout; },
            redraw);
        return true;
    }

    if(event == UHFCustomEventCloneWriteFail) {
        bool redraw = true;
        with_view_model(
            App->ViewClone,
            UHFReaderCloneModel * Model,
            { Model->phase = ClonePhaseFailed; },
            redraw);
        return true;
    }

    if(event == UHFCustomEventCloneAccessDenied) {
        bool redraw = true;
        with_view_model(
            App->ViewClone,
            UHFReaderCloneModel * Model,
            { Model->phase = ClonePhaseAccessDenied; },
            redraw);
        return true;
    }

    return false;
}

// ─── View lifecycle ───────────────────────────────────────────────────────────

void uhf_reader_view_clone_enter_callback(void* context) {
    UHFReaderApp* App = context;
    // Reset clone count on each fresh entry from the bank-selection screen
    bool redraw = true;
    with_view_model(
        App->ViewClone,
        UHFReaderCloneModel * Model,
        {
            Model->clone_count = 0;
            Model->clone_mask  = App->CloneMask;
        },
        redraw);
    notification_message(App->Notifications, &uhf_sequence_blink_start_cyan);
    start_clone_scan(App);
}

void uhf_reader_view_clone_exit_callback(void* context) {
    UHFReaderApp* App = context;
    // Stop any in-flight worker
    uhf_worker_stop(App->YRM100XWorker);
    notification_message(App->Notifications, &uhf_sequence_blink_stop);
}

// ─── Navigation ──────────────────────────────────────────────────────────────

uint32_t uhf_reader_navigation_clone_callback(void* context) {
    UNUSED(context);
    return UHFReaderViewCloneBanks;
}

// ─── Alloc / Free ─────────────────────────────────────────────────────────────

void view_clone_alloc(UHFReaderApp* App) {
    App->ViewClone = view_alloc();
    view_set_context(App->ViewClone, App);
    view_allocate_model(App->ViewClone, ViewModelTypeLocking, sizeof(UHFReaderCloneModel));
    view_set_draw_callback(App->ViewClone, uhf_reader_view_clone_draw_callback);
    view_set_input_callback(App->ViewClone, uhf_reader_view_clone_input_callback);
    view_set_custom_callback(App->ViewClone, uhf_reader_view_clone_custom_event_callback);
    view_set_enter_callback(App->ViewClone, uhf_reader_view_clone_enter_callback);
    view_set_exit_callback(App->ViewClone, uhf_reader_view_clone_exit_callback);
    view_set_previous_callback(App->ViewClone, uhf_reader_navigation_clone_callback);

    view_dispatcher_add_view(App->ViewDispatcher, UHFReaderViewClone, App->ViewClone);
}

void view_clone_free(UHFReaderApp* App) {
    view_dispatcher_remove_view(App->ViewDispatcher, UHFReaderViewClone);
    view_free(App->ViewClone);
    App->ViewClone = NULL;
}
