#include "yrm100x_uart.h"

/**
 * File that handles the uart for the YRM100
 * @author frux-c
*/
void uhf_uart_default_rx_callback(FuriHalSerialHandle *handle, FuriHalSerialRxEvent event, void* ctx) {
    UHFUart* uart = (UHFUart*)ctx;
    if((event & FuriHalSerialRxEventData) == FuriHalSerialRxEventData){
        uint8_t data = furi_hal_serial_async_rx(handle);
        // Frame-start realignment. In continuous multiple-polling the module streams
        // notification frames back-to-back. After a frame closes the buffer on 0x7E,
        // the main thread needs time to copy it out; bytes that arrive in the meantime
        // are dropped. Without realignment the next read would start mid-stream and the
        // frame would be mis-aligned (data[0] != 0xBB) and rejected.
        //
        // Reset only when the buffer is closed or empty. That realigns to a genuine
        // frame boundary while ensuring a 0xBB byte *inside* an EPC/PC payload (buffer
        // open and non-empty) is NOT mistaken for a frame start — this preserves the
        // single-poll behaviour exactly.
        if(data == UHF_UART_FRAME_START &&
           (uhf_is_buffer_closed(uart->buffer) || uhf_buffer_get_size(uart->buffer) == 0)){
            uhf_buffer_reset(uart->buffer);
        }
        if(uhf_is_buffer_closed(uart->buffer)){
            return;
        }
        uhf_buffer_append_single(uart->buffer, data);
        if(data == UHF_UART_FRAME_END){
            uhf_buffer_close(uart->buffer);
        }
        uhf_uart_tick_reset(uart);
    }
}

UHFUart* uhf_uart_alloc(){
    UHFUart *uart = (UHFUart*)malloc(sizeof(UHFUart));
    uart->bus = FuriHalBusUSART1;
    uart->handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    // uart->rx_buff_stream = furi_stream_buffer_alloc(UHF_UART_RX_BUFFER_SIZE, 1);
    uart->init_by_app = !furi_hal_bus_is_enabled(uart->bus);
    uart->tick = UHF_UART_WAIT_TICK;
    uart->baudrate = UHF_UART_DEFAULT_BAUDRATE;
    // expansion_disable();
    if(uart->init_by_app){
        FURI_LOG_E("UHF_UART", "UHF UART INIT BY APP");
        furi_hal_serial_init(uart->handle, uart->baudrate);
    }
    else{
        FURI_LOG_E("UHF_UART", "UHF UART INIT BY HAL");
    }
    uart->buffer = uhf_buffer_alloc(UHF_UART_RX_BUFFER_SIZE);
    furi_hal_serial_async_rx_start(uart->handle, uhf_uart_default_rx_callback, uart, false);
    return uart;
}   

void uhf_uart_free(UHFUart* uart){
    furi_assert(uart);
    // furi_assert(uart->thread);
    // furi_thread_flags_set(furi_thread_get_id(uart->thread), UHFUartWorkerExitingFlag);
    // furi_thread_join(uart->thread);
    // furi_thread_free(uart->thread);
    // furi_stream_buffer_free(uart->rx_buff_stream);
    uhf_buffer_free(uart->buffer);
    if(uart->init_by_app){
        furi_hal_serial_deinit(uart->handle);
    }
    furi_hal_serial_control_release(uart->handle);
    free(uart);
}

void uhf_uart_set_receive_byte_callback(UHFUart* uart, FuriHalSerialAsyncRxCallback callback, void *ctx, bool report_errors){
    furi_hal_serial_async_rx_start(uart->handle, callback, ctx, report_errors);
}

void uhf_uart_send(UHFUart* uart, uint8_t* data, size_t size){
    furi_hal_serial_tx(uart->handle, data, size);
}

void uhf_uart_send_wait(UHFUart* uart, uint8_t* data, size_t size){
    uhf_uart_send(uart, data, size);
    furi_hal_serial_tx_wait_complete(uart->handle);
    // furi_thread_flags_set(furi_thread_get_id(uart->thread), UHFUartWorkerWaitingDataFlag);
}

void uhf_uart_set_baudrate(UHFUart* uart, uint32_t baudrate){
    furi_hal_serial_set_br(uart->handle, baudrate);
    uart->baudrate = baudrate;
}

bool uhf_uart_tick(UHFUart* uart){
    if(uart->tick > 0){
        uart->tick--;
    }
    return uart->tick == 0;
}

void uhf_uart_tick_reset(UHFUart* uart){
    uart->tick = UHF_UART_WAIT_TICK;
}