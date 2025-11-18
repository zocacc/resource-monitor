/* monitor_tui.c - Terminal User Interface for Resource Monitor
 * Implementa interface interativa com ncurses
 * Autor: Grupo 15
 * Data: 14 de novembro de 2025
 */

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "monitor.h"
#include "network.h"
#include "namespace.h"
#include "cgroup.h"
#include "utils.h"

#define REFRESH_INTERVAL 1 // segundos
#define MAX_HISTORY 60     // manter 60 amostras de histórico

// Estrutura para histórico de métricas
typedef struct {
    double cpu_history[MAX_HISTORY];
    double mem_history[MAX_HISTORY];
    double io_read_history[MAX_HISTORY];
    double io_write_history[MAX_HISTORY];
    int history_index;
    int history_count;
} MetricsHistory;

// Cores
enum {
    COLOR_PAIR_TITLE = 1,
    COLOR_PAIR_HEADER,
    COLOR_PAIR_DATA,
    COLOR_PAIR_WARNING,
    COLOR_PAIR_ERROR,
    COLOR_PAIR_SUCCESS,
    COLOR_PAIR_GRAPH
};

// Inicializar cores
void init_colors(void) {
    start_color();
    init_pair(COLOR_PAIR_TITLE, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_PAIR_HEADER, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_DATA, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_PAIR_WARNING, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_SUCCESS, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_PAIR_GRAPH, COLOR_BLUE, COLOR_BLACK);
}

// Desenhar borda de janela
void draw_window_border(WINDOW *win, const char *title) {
    box(win, 0, 0);
    if (title) {
        wattron(win, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
        mvwprintw(win, 0, 2, " %s ", title);
        wattroff(win, COLOR_PAIR(COLOR_PAIR_TITLE) | A_BOLD);
    }
}

// Desenhar barra de progresso
void draw_progress_bar(WINDOW *win, int y, int x, int width, double percent) {
    int filled = (int)(width * percent / 100.0);
    if (filled > width) filled = width;
    
    wmove(win, y, x);
    wattron(win, COLOR_PAIR(COLOR_PAIR_GRAPH));
    for (int i = 0; i < filled; i++) {
        waddch(win, ACS_CKBOARD);
    }
    wattroff(win, COLOR_PAIR(COLOR_PAIR_GRAPH));
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    for (int i = filled; i < width; i++) {
        waddch(win, '-');
    }
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    // Mostrar porcentagem
    mvwprintw(win, y, x + width + 1, "%6.2f%%", percent);
}

// Desenhar gráfico ASCII simples
void draw_mini_graph(WINDOW *win, int y, int x, int width, int height, 
                     double *data, int data_count, const char *label) {
    if (data_count == 0) return;
    
    // Encontrar máximo
    double max_val = 0.0;
    for (int i = 0; i < data_count; i++) {
        if (data[i] > max_val) max_val = data[i];
    }
    if (max_val == 0.0) max_val = 1.0;
    
    // Desenhar label
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER));
    mvwprintw(win, y, x, "%s", label);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER));
    
    // Desenhar gráfico
    wattron(win, COLOR_PAIR(COLOR_PAIR_GRAPH));
    for (int i = 0; i < width && i < data_count; i++) {
        int bar_height = (int)(height * data[i] / max_val);
        if (bar_height > height) bar_height = height;
        
        for (int h = 0; h < bar_height; h++) {
            mvwaddch(win, y + height - h, x + i, ACS_CKBOARD);
        }
    }
    wattroff(win, COLOR_PAIR(COLOR_PAIR_GRAPH));
}

// Formatar bytes para legibilidade
void format_bytes(char *buf, size_t buf_size, unsigned long long bytes) {
    if (bytes < 1024) {
        snprintf(buf, buf_size, "%llu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buf, buf_size, "%.2f KB", bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        snprintf(buf, buf_size, "%.2f MB", bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buf, buf_size, "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

// Tela principal - Overview
void draw_overview_screen(WINDOW *win, ResourceData *snapshot, MetricsHistory *history) {
    int max_y, max_x __attribute__((unused));
    getmaxyx(win, max_y, max_x);
    
    werase(win);
    draw_window_border(win, "Resource Monitor - Overview");
    
    int row = 2;
    
    // Informações do processo
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvwprintw(win, row++, 2, "Process Information:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    mvwprintw(win, row++, 4, "PID: %d", snapshot->pid);
    mvwprintw(win, row++, 4, "Threads: %ld", snapshot->num_threads);
    
    // Formatar timestamp em hora legível
    char time_str[64];
    struct tm *tm_info = localtime(&snapshot->timestamp);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    mvwprintw(win, row++, 4, "Time: %s (unix: %ld)", time_str, snapshot->timestamp);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    row++;
    
    // CPU
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvwprintw(win, row++, 2, "CPU Usage:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    
    mvwprintw(win, row++, 4, "Total: ");
    draw_progress_bar(win, row - 1, 12, 40, snapshot->cpu_usage_percent);
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    mvwprintw(win, row++, 4, "User: %ld jiffies", snapshot->cpu_user);
    mvwprintw(win, row++, 4, "System: %ld jiffies", snapshot->cpu_system);
    mvwprintw(win, row++, 4, "Context Switches (vol): %ld", snapshot->voluntary_context_switches);
    mvwprintw(win, row++, 4, "Context Switches (invol): %ld", snapshot->nonvoluntary_context_switches);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    row++;
    
    // Memória
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvwprintw(win, row++, 2, "Memory Usage:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    
    char mem_buf[64];
    unsigned long long vsz_bytes = snapshot->memory_vsz * 1024ULL;
    unsigned long long rss_bytes = snapshot->memory_rss * 4096ULL; // Assumindo 4KB pages
    
    format_bytes(mem_buf, sizeof(mem_buf), vsz_bytes);
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    mvwprintw(win, row++, 4, "VSZ: %s", mem_buf);
    
    format_bytes(mem_buf, sizeof(mem_buf), rss_bytes);
    mvwprintw(win, row++, 4, "RSS: %s", mem_buf);
    
    format_bytes(mem_buf, sizeof(mem_buf), snapshot->memory_swap * 1024ULL);
    mvwprintw(win, row++, 4, "Swap: %s", mem_buf);
    
    mvwprintw(win, row++, 4, "Page Faults (minor): %ld", snapshot->page_faults_minor);
    mvwprintw(win, row++, 4, "Page Faults (major): %ld", snapshot->page_faults_major);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    row++;
    
    // I/O
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvwprintw(win, row++, 2, "I/O Statistics:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    
    format_bytes(mem_buf, sizeof(mem_buf), snapshot->io_read_bytes);
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    mvwprintw(win, row++, 4, "Read: %s (%.2f MB/s)", mem_buf, snapshot->io_read_rate / (1024.0 * 1024.0));
    
    format_bytes(mem_buf, sizeof(mem_buf), snapshot->io_write_bytes);
    mvwprintw(win, row++, 4, "Write: %s (%.2f MB/s)", mem_buf, snapshot->io_write_rate / (1024.0 * 1024.0));
    
    mvwprintw(win, row++, 4, "Syscalls (read): %lld", snapshot->io_read_syscalls);
    mvwprintw(win, row++, 4, "Syscalls (write): %lld", snapshot->io_write_syscalls);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    row++;
    
    // Network
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvwprintw(win, row++, 2, "Network Statistics:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    
    format_bytes(mem_buf, sizeof(mem_buf), snapshot->net_rx_bytes);
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    mvwprintw(win, row++, 4, "RX: %s (%lld packets)", mem_buf, snapshot->net_rx_packets);
    
    format_bytes(mem_buf, sizeof(mem_buf), snapshot->net_tx_bytes);
    mvwprintw(win, row++, 4, "TX: %s (%lld packets)", mem_buf, snapshot->net_tx_packets);
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    // Atualizar histórico
    if (history) {
        history->cpu_history[history->history_index] = snapshot->cpu_usage_percent;
        history->mem_history[history->history_index] = (double)rss_bytes / (1024.0 * 1024.0); // MB
        history->io_read_history[history->history_index] = snapshot->io_read_rate / (1024.0 * 1024.0); // MB/s
        history->io_write_history[history->history_index] = snapshot->io_write_rate / (1024.0 * 1024.0); // MB/s
        
        history->history_index = (history->history_index + 1) % MAX_HISTORY;
        if (history->history_count < MAX_HISTORY) {
            history->history_count++;
        }
    }
    
    // Rodapé com instruções
    wattron(win, COLOR_PAIR(COLOR_PAIR_WARNING));
    mvwprintw(win, max_y - 2, 2, "[q] Quit  [r] Refresh  [h] Help");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_WARNING));
    
    wrefresh(win);
}

// Tela de ajuda
void draw_help_screen(WINDOW *win) {
    werase(win);
    draw_window_border(win, "Help - Keyboard Shortcuts");
    
    int row = 2;
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvwprintw(win, row++, 2, "Navigation:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    mvwprintw(win, row++, 4, "q - Quit application");
    mvwprintw(win, row++, 4, "r - Force refresh");
    mvwprintw(win, row++, 4, "h - Show this help");
    mvwprintw(win, row++, 4, "ESC - Return to overview");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    row++;
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    mvwprintw(win, row++, 2, "About:");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_HEADER) | A_BOLD);
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_DATA));
    mvwprintw(win, row++, 4, "Resource Monitor TUI v1.0");
    mvwprintw(win, row++, 4, "Grupo 15 - Sistemas Operacionais");
    mvwprintw(win, row++, 4, "Data: Novembro 2025");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_DATA));
    
    row += 2;
    
    wattron(win, COLOR_PAIR(COLOR_PAIR_WARNING));
    mvwprintw(win, row++, 2, "Press any key to return...");
    wattroff(win, COLOR_PAIR(COLOR_PAIR_WARNING));
    
    wrefresh(win);
    wgetch(win);
}

// Loop principal da TUI
int run_tui(pid_t pid, int interval, int duration) {
    ResourceData snapshot = {0};
    ResourceData prev_snapshot = {0};
    MetricsHistory history = {0};
    bool first_sample = true;
    long ticks_per_second = sysconf(_SC_CLK_TCK);
    
    // Determinar modo de operação
    bool timed_mode = (duration > 0);
    int refresh_interval = (interval > 0) ? interval : REFRESH_INTERVAL;
    
    // Alocar histórico para modo com tempo determinado
    ResourceData *data_history = NULL;
    int num_samples = 0;
    int current_sample = 0;
    
    if (timed_mode) {
        num_samples = duration / refresh_interval;
        if (num_samples <= 0) {
            fprintf(stderr, "Erro: Duração deve ser maior que o intervalo.\n");
            return -1;
        }
        data_history = malloc(num_samples * sizeof(ResourceData));
        if (data_history == NULL) {
            perror("Falha ao alocar memória para o histórico");
            return -1;
        }
    }
    
    // Inicializar ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); // Non-blocking getch
    
    // Inicializar cores
    if (has_colors()) {
        init_colors();
    }
    
    // Criar janela principal
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *main_win = newwin(max_y, max_x, 0, 0);
    keypad(main_win, TRUE);
    wtimeout(main_win, 100); // 100ms timeout para wgetch
    
    // Loop principal
    int running = 1;
    time_t last_refresh = 0;
    time_t start_time = time(NULL);
    
    while (running) {
        time_t now = time(NULL);
        
        // Verificar se alcançou duração máxima no modo temporizado
        if (timed_mode && (now - start_time) >= duration) {
            running = 0;
            break;
        }
        
        // Atualizar dados se passou tempo suficiente
        if (now - last_refresh >= refresh_interval) {
            // Coletar dados do processo
            snapshot.pid = pid;
            snapshot.timestamp = now;
            
            if (get_cpu_data(pid, &snapshot) &&
                get_memory_data(pid, &snapshot) &&
                get_io_data(pid, &snapshot) &&
                get_network_data(pid, &snapshot)) {
                
                // Calcular CPU%, taxas de I/O e taxas de rede
                if (first_sample) {
                    snapshot.cpu_usage_percent = 0.0;
                    snapshot.io_read_rate = 0.0;
                    snapshot.io_write_rate = 0.0;
                    first_sample = false;
                } else {
                    // Calcular delta de tempo
                    double time_delta_sec = difftime(snapshot.timestamp, prev_snapshot.timestamp);
                    if (time_delta_sec <= 0) time_delta_sec = REFRESH_INTERVAL;
                    
                    // Calcular CPU%
                    long total_cpu_time_diff = (snapshot.cpu_user + snapshot.cpu_system) - 
                                              (prev_snapshot.cpu_user + prev_snapshot.cpu_system);
                    snapshot.cpu_usage_percent = 100.0 * (total_cpu_time_diff / (double)ticks_per_second) / time_delta_sec;
                    
                    // Calcular taxas de I/O
                    snapshot.io_read_rate = (snapshot.io_read_bytes - prev_snapshot.io_read_bytes) / time_delta_sec;
                    snapshot.io_write_rate = (snapshot.io_write_bytes - prev_snapshot.io_write_bytes) / time_delta_sec;
                }
                
                // Armazenar dados anteriores para próximo cálculo
                prev_snapshot = snapshot;
                
                // Armazenar no histórico se em modo temporizado
                if (timed_mode && current_sample < num_samples) {
                    data_history[current_sample] = snapshot;
                    current_sample++;
                }
                
                // Desenhar tela
                draw_overview_screen(main_win, &snapshot, &history);
                last_refresh = now;
            } else {
                // Processo pode ter terminado
                werase(main_win);
                draw_window_border(main_win, "Error");
                
                wattron(main_win, COLOR_PAIR(COLOR_PAIR_ERROR) | A_BOLD);
                mvwprintw(main_win, 2, 2, "Failed to read process statistics!");
                mvwprintw(main_win, 3, 2, "Process PID %d may have terminated.", pid);
                wattroff(main_win, COLOR_PAIR(COLOR_PAIR_ERROR) | A_BOLD);
                
                wattron(main_win, COLOR_PAIR(COLOR_PAIR_WARNING));
                mvwprintw(main_win, 5, 2, "Press 'q' to quit...");
                wattroff(main_win, COLOR_PAIR(COLOR_PAIR_WARNING));
                
                wrefresh(main_win);
            }
        }
        
        // Processar entrada do teclado
        int ch = wgetch(main_win);
        switch (ch) {
            case 'q':
            case 'Q':
                running = 0;
                break;
            case 'r':
            case 'R':
                last_refresh = 0; // Forçar refresh imediato
                break;
            case 'h':
            case 'H':
                draw_help_screen(main_win);
                last_refresh = 0; // Forçar refresh após voltar
                break;
            case 27: // ESC
                // Voltar para overview (já é a tela padrão)
                break;
        }
        
        usleep(50000); // 50ms sleep para não consumir CPU
    }
    
    // Cleanup
    delwin(main_win);
    endwin();
    
    // Exportar dados se em modo temporizado
    if (timed_mode && data_history != NULL && current_sample > 0) {
        export_to_json(data_history, current_sample, "output/monitor_output.json");
    }
    
    // Liberar memória
    if (data_history != NULL) {
        free(data_history);
    }
    
    return 0;
}
