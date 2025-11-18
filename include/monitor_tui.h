/* monitor_tui.h - Header para Terminal User Interface
 * Autor: Grupo 15
 */

#ifndef MONITOR_TUI_H
#define MONITOR_TUI_H

#include <sys/types.h>

// Executa a interface TUI para monitorar um processo
// Se interval e duration forem 0, executa em modo interativo (infinito)
// Se duration > 0, executa por tempo determinado e gera output JSON
// Retorna 0 em sucesso, -1 em erro
int run_tui(pid_t pid, int interval, int duration);

#endif // MONITOR_TUI_H
