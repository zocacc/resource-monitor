#!/usr/bin/env python3
"""
visualize.py - Visualização de Dados de Monitoramento
Gera gráficos a partir dos arquivos JSON e CSV exportados pelo monitor
Suporta:
- Dados de monitoramento contínuo (JSON)
- Experimentos 1-5 (CSV)
"""

import json
import sys
import os
import csv
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
    from datetime import datetime
    import numpy as np
except ImportError:
    print("Erro: matplotlib ou numpy não instalado.")
    print("Instale com: pip install matplotlib numpy")
    sys.exit(1)


def load_monitor_data(filename):
    """Carrega dados do arquivo JSON de monitoramento"""
    try:
        with open(filename, 'r') as f:
            data = json.load(f)
        return data
    except FileNotFoundError:
        print(f"Erro: Arquivo '{filename}' não encontrado.")
        sys.exit(1)
    except json.JSONDecodeError:
        print(f"Erro: Arquivo '{filename}' não é um JSON válido.")
        sys.exit(1)


def plot_cpu_usage(data, output_dir):
    """Plota o uso de CPU ao longo do tempo"""
    if not data:
        print("Aviso: Nenhum dado para plotar CPU.")
        return
    
    timestamps = [datetime.fromtimestamp(d['timestamp']) for d in data]
    cpu_percent = [d['cpu_usage_percent'] for d in data]
    
    plt.figure(figsize=(12, 6))
    plt.plot(timestamps, cpu_percent, marker='o', linestyle='-', linewidth=2, markersize=4)
    plt.title('Uso de CPU ao Longo do Tempo', fontsize=16, fontweight='bold')
    plt.xlabel('Timestamp', fontsize=12)
    plt.ylabel('CPU Usage (%)', fontsize=12)
    plt.grid(True, alpha=0.3)
    plt.xticks(rotation=45)
    plt.tight_layout()
    
    output_file = os.path.join(output_dir, 'cpu_usage.png')
    plt.savefig(output_file, dpi=150)
    print(f" Gráfico de CPU salvo em: {output_file}")
    plt.close()


def plot_memory_usage(data, output_dir):
    """Plota o uso de memória ao longo do tempo"""
    if not data:
        print("Aviso: Nenhum dado para plotar memória.")
        return
    
    timestamps = [datetime.fromtimestamp(d['timestamp']) for d in data]
    vsz_mb = [d['memory_vsz_kb'] / 1024 for d in data]  # Convert KB to MB
    rss_mb = [d['memory_rss_pages'] * 4 / 1024 for d in data]  # Assume 4KB pages, convert to MB
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))
    
    # VSZ (Virtual Size)
    ax1.plot(timestamps, vsz_mb, marker='s', linestyle='-', color='blue', linewidth=2, markersize=4, label='VSZ')
    ax1.set_title('Memória Virtual (VSZ) ao Longo do Tempo', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Timestamp', fontsize=12)
    ax1.set_ylabel('VSZ (MB)', fontsize=12)
    ax1.grid(True, alpha=0.3)
    ax1.legend()
    ax1.tick_params(axis='x', rotation=45)
    
    # RSS (Resident Set Size)
    ax2.plot(timestamps, rss_mb, marker='^', linestyle='-', color='green', linewidth=2, markersize=4, label='RSS')
    ax2.set_title('Memória Residente (RSS) ao Longo do Tempo', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Timestamp', fontsize=12)
    ax2.set_ylabel('RSS (MB)', fontsize=12)
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    ax2.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    output_file = os.path.join(output_dir, 'memory_usage.png')
    plt.savefig(output_file, dpi=150)
    print(f" Gráfico de Memória salvo em: {output_file}")
    plt.close()


def plot_io_rates(data, output_dir):
    """Plota as taxas de I/O ao longo do tempo"""
    if not data:
        print("Aviso: Nenhum dado para plotar I/O.")
        return
    
    timestamps = [datetime.fromtimestamp(d['timestamp']) for d in data]
    read_rate_mbps = [d['io_read_rate_bps'] / (1024 * 1024) for d in data]  # Convert to MB/s
    write_rate_mbps = [d['io_write_rate_bps'] / (1024 * 1024) for d in data]
    
    plt.figure(figsize=(12, 6))
    plt.plot(timestamps, read_rate_mbps, marker='o', linestyle='-', color='royalblue', 
             linewidth=2, markersize=4, label='Read Rate')
    plt.plot(timestamps, write_rate_mbps, marker='s', linestyle='-', color='crimson', 
             linewidth=2, markersize=4, label='Write Rate')
    plt.title('Taxas de I/O ao Longo do Tempo', fontsize=16, fontweight='bold')
    plt.xlabel('Timestamp', fontsize=12)
    plt.ylabel('Taxa (MB/s)', fontsize=12)
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3)
    plt.xticks(rotation=45)
    plt.tight_layout()
    
    output_file = os.path.join(output_dir, 'io_rates.png')
    plt.savefig(output_file, dpi=150)
    print(f" Gráfico de I/O salvo em: {output_file}")
    plt.close()


def plot_network_traffic(data, output_dir):
    """Plota o tráfego de rede ao longo do tempo"""
    if not data:
        print("Aviso: Nenhum dado para plotar rede.")
        return
    
    timestamps = [datetime.fromtimestamp(d['timestamp']) for d in data]
    rx_mb = [d['net_rx_bytes'] / (1024 * 1024) for d in data]  # Convert to MB
    tx_mb = [d['net_tx_bytes'] / (1024 * 1024) for d in data]
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))
    
    # Received bytes
    ax1.plot(timestamps, rx_mb, marker='o', linestyle='-', color='dodgerblue', 
             linewidth=2, markersize=4, label='RX (Received)')
    ax1.set_title('Tráfego de Rede Recebido (RX) ao Longo do Tempo', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Timestamp', fontsize=12)
    ax1.set_ylabel('Bytes Recebidos (MB)', fontsize=12)
    ax1.grid(True, alpha=0.3)
    ax1.legend()
    ax1.tick_params(axis='x', rotation=45)
    
    # Transmitted bytes
    ax2.plot(timestamps, tx_mb, marker='s', linestyle='-', color='darkorange', 
             linewidth=2, markersize=4, label='TX (Transmitted)')
    ax2.set_title('Tráfego de Rede Transmitido (TX) ao Longo do Tempo', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Timestamp', fontsize=12)
    ax2.set_ylabel('Bytes Transmitidos (MB)', fontsize=12)
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    ax2.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    output_file = os.path.join(output_dir, 'network_traffic.png')
    plt.savefig(output_file, dpi=150)
    print(f" Gráfico de Rede salvo em: {output_file}")
    plt.close()


def plot_all_metrics(data, output_dir):
    """Gera um dashboard com todas as métricas"""
    if not data:
        print("Aviso: Nenhum dado para plotar dashboard.")
        return
    
    timestamps = [datetime.fromtimestamp(d['timestamp']) for d in data]
    
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    fig.suptitle('Dashboard de Monitoramento de Recursos', fontsize=18, fontweight='bold')
    
    # CPU
    cpu_percent = [d['cpu_usage_percent'] for d in data]
    axes[0, 0].plot(timestamps, cpu_percent, marker='o', color='royalblue', linewidth=2, markersize=3)
    axes[0, 0].set_title('CPU Usage (%)', fontsize=13, fontweight='bold')
    axes[0, 0].set_ylabel('CPU %', fontsize=11)
    axes[0, 0].grid(True, alpha=0.3)
    axes[0, 0].tick_params(axis='x', rotation=45)
    
    # Memory RSS
    rss_mb = [d['memory_rss_pages'] * 4 / 1024 for d in data]
    axes[0, 1].plot(timestamps, rss_mb, marker='^', color='green', linewidth=2, markersize=3)
    axes[0, 1].set_title('Memory RSS (MB)', fontsize=13, fontweight='bold')
    axes[0, 1].set_ylabel('RSS (MB)', fontsize=11)
    axes[0, 1].grid(True, alpha=0.3)
    axes[0, 1].tick_params(axis='x', rotation=45)
    
    # I/O Rates
    read_rate = [d['io_read_rate_bps'] / (1024 * 1024) for d in data]
    write_rate = [d['io_write_rate_bps'] / (1024 * 1024) for d in data]
    axes[1, 0].plot(timestamps, read_rate, marker='o', color='blue', linewidth=2, markersize=3, label='Read')
    axes[1, 0].plot(timestamps, write_rate, marker='s', color='red', linewidth=2, markersize=3, label='Write')
    axes[1, 0].set_title('I/O Rates (MB/s)', fontsize=13, fontweight='bold')
    axes[1, 0].set_ylabel('Rate (MB/s)', fontsize=11)
    axes[1, 0].legend()
    axes[1, 0].grid(True, alpha=0.3)
    axes[1, 0].tick_params(axis='x', rotation=45)
    
    # Network
    rx_mb = [d['net_rx_bytes'] / (1024 * 1024) for d in data]
    tx_mb = [d['net_tx_bytes'] / (1024 * 1024) for d in data]
    axes[1, 1].plot(timestamps, rx_mb, marker='o', color='dodgerblue', linewidth=2, markersize=3, label='RX')
    axes[1, 1].plot(timestamps, tx_mb, marker='s', color='orange', linewidth=2, markersize=3, label='TX')
    axes[1, 1].set_title('Network Traffic (MB)', fontsize=13, fontweight='bold')
    axes[1, 1].set_ylabel('Traffic (MB)', fontsize=11)
    axes[1, 1].legend()
    axes[1, 1].grid(True, alpha=0.3)
    axes[1, 1].tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    output_file = os.path.join(output_dir, 'dashboard.png')
    plt.savefig(output_file, dpi=150)
    print(f" Dashboard completo salvo em: {output_file}")
    plt.close()


def main():
    if len(sys.argv) < 2:
        print("Uso: python3 visualize.py <arquivo_json_ou_csv> [diretorio_saida]")
        print("\nExemplos:")
        print("  Monitoramento contínuo:")
        print("    python3 visualize.py output/monitor_output.json output/graphs")
        print("\n  Experimentos:")
        print("    python3 visualize.py output/experiment1_overhead.csv output/graphs")
        print("    python3 visualize.py output/experiment3_cpu_throttling.csv output/graphs")
        print("    python3 visualize.py output/experiment4_memory_limit.csv output/graphs")
        print("    python3 visualize.py output/experiment5_io_limit.csv output/graphs")
        print("\n  Gerar TODOS os gráficos de experimentos:")
        print("    python3 visualize.py --experiments output/graphs")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "output/graphs"
    
    # Criar diretório de saída se não existir
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    
    # Modo especial: gerar todos os experimentos
    if input_file == "--experiments":
        print("\n Modo: Gerar visualizações de TODOS os experimentos\n")
        generate_all_experiments(output_dir)
        return
    
    # Detectar tipo de arquivo
    if input_file.endswith('.json'):
        # Verificar se é experimento 2 (namespace)
        if 'exp2' in input_file or 'namespace' in input_file:
            print(f"\nCarregando dados de experimento de: {input_file}")
            plot_experiment2(input_file, output_dir)
        else:
            print(f"\nCarregando dados de monitoramento de: {input_file}")
            data = load_monitor_data(input_file)
            print(f"OK - {len(data)} amostras carregadas\n")
            
            print("Gerando gráficos...")
            plot_cpu_usage(data, output_dir)
            plot_memory_usage(data, output_dir)
            plot_io_rates(data, output_dir)
            plot_network_traffic(data, output_dir)
            plot_all_metrics(data, output_dir)
        
    elif input_file.endswith('.csv'):
        print(f"\n Carregando dados de experimento de: {input_file}")
        
        # Detectar qual experimento baseado no nome do arquivo
        if 'experiment1_overhead' in input_file or 'exp1' in input_file:
            plot_experiment1(input_file, output_dir)
        elif 'experiment3_cpu' in input_file or 'exp3' in input_file:
            plot_experiment3(input_file, output_dir)
        elif 'experiment4_memory' in input_file or 'exp4' in input_file:
            plot_experiment4(input_file, output_dir)
        elif 'experiment5_io' in input_file or 'exp5' in input_file:
            plot_experiment5(input_file, output_dir)
        else:
            print(f"Erro: Tipo de experimento não reconhecido em '{input_file}'")
            sys.exit(1)
    else:
        print(f"Erro: Formato de arquivo não suportado. Use .json ou .csv")
        sys.exit(1)
    
    print(f"\n Visualização concluída! Gráficos salvos em: {output_dir}/")


def load_csv_data(filename):
    """Carrega dados de arquivo CSV"""
    data = []
    with open(filename, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            data.append(row)
    return data


def plot_experiment1(csv_file, output_dir):
    """
    Experimento 1: Overhead de Monitoramento
    Gráficos:
    - Overhead de tempo de execução vs intervalo de sampling
    - Overhead de CPU vs intervalo de sampling
    - Context switches vs intervalo de sampling
    """
    print(" Gerando gráficos do Experimento 1 (Overhead de Monitoramento)...")
    
    data = load_csv_data(csv_file)
    
    # Filtrar baseline
    baseline = next((d for d in data if float(d['sampling_interval_ms']) == 0), None)
    monitored = [d for d in data if float(d['sampling_interval_ms']) > 0]
    
    intervals = [float(d['sampling_interval_ms']) for d in monitored]
    time_overhead = [float(d['time_overhead_percent']) for d in monitored]
    cpu_overhead = [float(d['cpu_overhead_percent']) for d in monitored]
    ctx_switches_delta = [int(d['ctx_switches_delta']) for d in monitored]
    
    # Gráfico 1: Overhead de tempo e CPU
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    ax1.plot(intervals, time_overhead, marker='o', linewidth=2, markersize=8, color='royalblue')
    ax1.axhline(y=0, color='gray', linestyle='--', alpha=0.5)
    ax1.set_title('Overhead de Tempo de Execução', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Intervalo de Sampling (ms)', fontsize=12)
    ax1.set_ylabel('Overhead (%)', fontsize=12)
    ax1.grid(True, alpha=0.3)
    ax1.set_xscale('log')
    
    ax2.plot(intervals, cpu_overhead, marker='s', linewidth=2, markersize=8, color='crimson')
    ax2.axhline(y=0, color='gray', linestyle='--', alpha=0.5)
    ax2.set_title('Overhead de CPU', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Intervalo de Sampling (ms)', fontsize=12)
    ax2.set_ylabel('Overhead (%)', fontsize=12)
    ax2.grid(True, alpha=0.3)
    ax2.set_xscale('log')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp1_overhead.png', dpi=150, bbox_inches='tight')
    print(f"   exp1_overhead.png")
    plt.close()
    
    # Gráfico 2: Context Switches
    plt.figure(figsize=(10, 6))
    plt.bar(range(len(intervals)), ctx_switches_delta, color='orange', alpha=0.7, edgecolor='black')
    plt.xticks(range(len(intervals)), [f'{int(i)} ms' for i in intervals])
    plt.title('Context Switches Adicionais por Intervalo', fontsize=14, fontweight='bold')
    plt.xlabel('Intervalo de Sampling', fontsize=12)
    plt.ylabel('Context Switches Delta', fontsize=12)
    plt.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp1_context_switches.png', dpi=150, bbox_inches='tight')
    print(f"   exp1_context_switches.png")
    plt.close()
    
    # Gráfico 3: Comparação de tempo de execução
    exec_times = [float(d['execution_time_sec']) for d in data]
    labels = ['Baseline'] + [f'{int(i)} ms' for i in intervals]
    
    plt.figure(figsize=(10, 6))
    bars = plt.bar(range(len(exec_times)), exec_times, color=['green'] + ['steelblue'] * len(intervals),
                   alpha=0.7, edgecolor='black')
    plt.xticks(range(len(labels)), labels, rotation=15)
    plt.title('Tempo de Execução por Configuração', fontsize=14, fontweight='bold')
    plt.xlabel('Configuração', fontsize=12)
    plt.ylabel('Tempo de Execução (segundos)', fontsize=12)
    plt.grid(True, alpha=0.3, axis='y')
    
    # Adicionar valores nas barras
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}s', ha='center', va='bottom', fontsize=9)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp1_execution_time.png', dpi=150, bbox_inches='tight')
    print(f"   exp1_execution_time.png")
    plt.close()


def plot_experiment2(json_file, output_dir):
    """
    Experimento 2: Isolamento via Namespaces
    Gráficos:
    - Comparação de visibilidade de recursos
    - Tempos de criação de namespaces
    - Efetividade do isolamento
    """
    print("Gerando gráficos do Experimento 2 (Isolamento via Namespaces)...")
    
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    # A estrutura real é data['isolation_tests'] que é um dict de namespaces
    isolation_tests = data.get('isolation_tests', {})
    if not isolation_tests:
        print("  Aviso: Nenhum dado encontrado no arquivo JSON")
        return
    
    # Extrair dados da estrutura real
    namespaces = []
    creation_times = []
    isolation_status = []
    host_resources = []
    ns_resources = []
    
    # Mapear os namespaces (excluindo 'multiple_namespaces' para os gráficos individuais)
    ns_mapping = {
        'pid_namespace': ('PID', 'host_processes', 'processes_visible'),
        'net_namespace': ('NET', 'host_interfaces', 'interfaces_visible'),
        'uts_namespace': ('UTS', None, None),
        'ipc_namespace': ('IPC', 'host_ipc_queues', 'ipc_queues_visible'),
        'mount_namespace': ('MOUNT', None, None)
    }
    
    for ns_key, (ns_name, host_key, ns_key_resource) in ns_mapping.items():
        if ns_key in isolation_tests:
            test = isolation_tests[ns_key]
            namespaces.append(ns_name)
            creation_times.append(test.get('creation_time_us', 0) / 1000)  # Converter para ms
            isolation_status.append(1 if test.get('isolated', False) else 0)
            
            # Recursos do host e namespace (se disponíveis)
            if host_key and ns_key_resource:
                host_resources.append(test.get(host_key, 0))
                ns_resources.append(test.get(ns_key_resource, 0))
            else:
                host_resources.append(0)
                ns_resources.append(0)
    
    # Gráfico 1: Tempos de criação
    plt.figure(figsize=(10, 6))
    colors = ['green' if isolated else 'red' for isolated in isolation_status]
    bars = plt.bar(range(len(namespaces)), creation_times, color=colors, alpha=0.7, edgecolor='black')
    plt.xticks(range(len(namespaces)), namespaces, rotation=15)
    plt.title('Tempo de Criacao de Namespaces', fontsize=14, fontweight='bold')
    plt.xlabel('Tipo de Namespace', fontsize=12)
    plt.ylabel('Tempo (ms)', fontsize=12)
    plt.grid(True, alpha=0.3, axis='y')
    
    for i, bar in enumerate(bars):
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}ms', ha='center', va='bottom', fontsize=9)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp2_creation_time.png', dpi=150, bbox_inches='tight')
    print(f"   exp2_creation_time.png")
    plt.close()
    
    # Gráfico 2: Comparação de recursos visíveis (apenas para namespaces com dados)
    namespaces_with_resources = []
    host_with_resources = []
    ns_with_resources = []
    
    for i, ns in enumerate(namespaces):
        if host_resources[i] > 0 or ns_resources[i] > 0:
            namespaces_with_resources.append(ns)
            host_with_resources.append(host_resources[i])
            ns_with_resources.append(ns_resources[i])
    
    if namespaces_with_resources:
        fig, ax = plt.subplots(figsize=(12, 6))
        x_pos = np.arange(len(namespaces_with_resources))
        width = 0.35
        
        ax.bar(x_pos - width/2, host_with_resources, width, label='Host', color='lightcoral', edgecolor='black')
        ax.bar(x_pos + width/2, ns_with_resources, width, label='Namespace', color='lightblue', edgecolor='black')
        
        ax.set_xlabel('Tipo de Namespace', fontsize=12)
        ax.set_ylabel('Recursos Visiveis', fontsize=12)
        ax.set_title('Recursos Visiveis: Host vs Namespace', fontsize=14, fontweight='bold')
        ax.set_xticks(x_pos)
        ax.set_xticklabels(namespaces_with_resources, rotation=15)
        ax.legend()
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        plt.savefig(f'{output_dir}/exp2_resource_visibility.png', dpi=150, bbox_inches='tight')
        print(f"   exp2_resource_visibility.png")
        plt.close()
    
    # Gráfico 3: Status de isolamento
    isolated_count = sum(isolation_status)
    not_isolated_count = len(isolation_status) - isolated_count
    
    plt.figure(figsize=(8, 8))
    sizes = [isolated_count, not_isolated_count]
    labels = [f'Isolado ({isolated_count})', f'Nao Isolado ({not_isolated_count})']
    colors_pie = ['#90EE90', '#FFB6C6']
    explode = (0.1, 0) if not_isolated_count > 0 else (0, 0)
    
    if not_isolated_count > 0:
        plt.pie(sizes, explode=explode, labels=labels, colors=colors_pie, autopct='%1.1f%%',
                shadow=True, startangle=90, textprops={'fontsize': 12})
    else:
        # Se todos estão isolados, mostrar apenas uma fatia
        plt.pie([isolated_count], labels=[f'Todos Isolados ({isolated_count})'], colors=['#90EE90'],
                autopct='%1.0f%%', shadow=True, startangle=90, textprops={'fontsize': 12})
    
    plt.title('Efetividade do Isolamento', fontsize=14, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp2_isolation_effectiveness.png', dpi=150, bbox_inches='tight')
    print(f"   exp2_isolation_effectiveness.png")
    plt.close()


def plot_experiment3(csv_file, output_dir):
    """
    Experimento 3: CPU Throttling
    Gráficos:
    - CPU% medido vs limite configurado
    - Desvio percentual por limite
    - Throughput vs limite de CPU
    """
    print(" Gerando gráficos do Experimento 3 (CPU Throttling)...")
    
    data = load_csv_data(csv_file)
    
    # Filtrar baseline
    baseline = next((d for d in data if float(d['cpu_limit_cores']) < 0), None)
    limited = [d for d in data if float(d['cpu_limit_cores']) > 0]
    
    limits = [float(d['cpu_limit_cores']) * 100 for d in limited]  # Converter para %
    measured = [float(d['measured_cpu_percent']) for d in limited]
    deviations = [float(d['deviation_percent']) for d in limited]
    throughputs = [float(d['throughput_iter_per_sec']) for d in limited]
    
    # Gráfico 1: CPU Medido vs Configurado
    fig, ax = plt.subplots(figsize=(10, 6))
    
    x_pos = np.arange(len(limits))
    width = 0.35
    
    ax.bar(x_pos - width/2, limits, width, label='Limite Configurado', color='lightblue', edgecolor='black')
    ax.bar(x_pos + width/2, measured, width, label='CPU Medido', color='salmon', edgecolor='black')
    ax.plot(x_pos, limits, 'k--', alpha=0.5, label='Linha Ideal')
    
    ax.set_xlabel('Limite (cores)', fontsize=12)
    ax.set_ylabel('CPU (%)', fontsize=12)
    ax.set_title('CPU Medido vs Limite Configurado', fontsize=14, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels([f'{l/100:.2f}' for l in limits])
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp3_cpu_comparison.png', dpi=150, bbox_inches='tight')
    print(f"   exp3_cpu_comparison.png")
    plt.close()
    
    # Gráfico 2: Desvio Percentual
    plt.figure(figsize=(10, 6))
    colors = ['green' if abs(d) < 5 else 'orange' if abs(d) < 15 else 'red' for d in deviations]
    bars = plt.bar(range(len(limits)), deviations, color=colors, alpha=0.7, edgecolor='black')
    plt.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
    plt.axhline(y=5, color='orange', linestyle='--', alpha=0.5, label='±5% (Alta Precisão)')
    plt.axhline(y=-5, color='orange', linestyle='--', alpha=0.5)
    plt.xticks(range(len(limits)), [f'{l/100:.2f} cores' for l in limits])
    plt.title('Desvio Percentual por Limite', fontsize=14, fontweight='bold')
    plt.xlabel('Limite de CPU', fontsize=12)
    plt.ylabel('Desvio (%)', fontsize=12)
    plt.legend()
    plt.grid(True, alpha=0.3, axis='y')
    
    # Adicionar valores
    for i, bar in enumerate(bars):
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:+.1f}%', ha='center', va='bottom' if height > 0 else 'top', fontsize=9)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp3_deviation.png', dpi=150, bbox_inches='tight')
    print(f"   exp3_deviation.png")
    plt.close()
    
    # Gráfico 3: Throughput vs Limite
    baseline_throughput = float(baseline['throughput_iter_per_sec']) if baseline else max(throughputs)
    
    plt.figure(figsize=(10, 6))
    plt.plot([l/100 for l in limits], throughputs, marker='o', linewidth=2, markersize=10, color='royalblue')
    plt.axhline(y=baseline_throughput, color='green', linestyle='--', label=f'Baseline ({baseline_throughput:.0f} iter/s)')
    plt.title('Throughput vs Limite de CPU', fontsize=14, fontweight='bold')
    plt.xlabel('Limite de CPU (cores)', fontsize=12)
    plt.ylabel('Throughput (iterações/s)', fontsize=12)
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp3_throughput.png', dpi=150, bbox_inches='tight')
    print(f"   exp3_throughput.png")
    plt.close()


def plot_experiment4(csv_file, output_dir):
    """
    Experimento 4: Limitação de Memória
    Gráficos:
    - Progresso de alocação de memória
    - Status de cada tentativa
    - Uso atual vs pico
    """
    print(" Gerando gráficos do Experimento 4 (Limitação de Memória)...")
    
    data = load_csv_data(csv_file)
    
    steps = [int(d['step']) for d in data]
    targets = [int(d['target_mb']) for d in data]
    current = [int(d['current_mb']) for d in data]
    peaks = [int(d['peak_mb']) for d in data]
    success = [int(d['success']) for d in data]
    
    # Gráfico 1: Progresso de Alocação
    fig, ax = plt.subplots(figsize=(12, 6))
    
    ax.plot(steps, targets, marker='o', linewidth=2, markersize=8, color='blue', label='Memória Tentada')
    ax.plot(steps, peaks, marker='s', linewidth=2, markersize=8, color='red', label='Pico de Memória')
    ax.axhline(y=100, color='orange', linestyle='--', linewidth=2, label='Limite Configurado (100 MB)')
    
    # Marcar falhas
    failures = [i for i, s in enumerate(success) if s == 0]
    if failures:
        fail_steps = [steps[i] for i in failures]
        fail_targets = [targets[i] for i in failures]
        ax.scatter(fail_steps, fail_targets, color='red', s=200, marker='x', linewidths=3, 
                  label='Falhas de Alocação', zorder=5)
    
    ax.set_xlabel('Passo de Alocação', fontsize=12)
    ax.set_ylabel('Memória (MB)', fontsize=12)
    ax.set_title('Progresso de Alocação de Memória', fontsize=14, fontweight='bold')
    ax.legend(loc='best')
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp4_allocation_progress.png', dpi=150, bbox_inches='tight')
    print(f"   exp4_allocation_progress.png")
    plt.close()
    
    # Gráfico 2: Status de cada tentativa
    plt.figure(figsize=(12, 6))
    colors = ['green' if s == 1 else 'red' for s in success]
    bars = plt.bar(steps, targets, color=colors, alpha=0.7, edgecolor='black')
    plt.axhline(y=100, color='orange', linestyle='--', linewidth=2, label='Limite (100 MB)')
    plt.xlabel('Passo', fontsize=12)
    plt.ylabel('Memória Tentada (MB)', fontsize=12)
    plt.title('Status de Alocação por Passo', fontsize=14, fontweight='bold')
    plt.legend(['Sucesso', 'Falha', 'Limite'])
    plt.grid(True, alpha=0.3, axis='y')
    
    # Adicionar status nos topos
    for i, (bar, s) in enumerate(zip(bars, success)):
        status_text = '' if s == 1 else ''
        plt.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                status_text, ha='center', va='bottom', fontsize=12, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp4_allocation_status.png', dpi=150, bbox_inches='tight')
    print(f"   exp4_allocation_status.png")
    plt.close()
    
    # Gráfico 3: Pico vs Uso Atual
    fig, ax = plt.subplots(figsize=(10, 6))
    
    x_pos = np.arange(len(steps))
    width = 0.35
    
    ax.bar(x_pos - width/2, current, width, label='Uso Atual', color='lightblue', edgecolor='black')
    ax.bar(x_pos + width/2, peaks, width, label='Pico', color='coral', edgecolor='black')
    
    ax.set_xlabel('Passo', fontsize=12)
    ax.set_ylabel('Memória (MB)', fontsize=12)
    ax.set_title('Uso Atual vs Pico de Memória', fontsize=14, fontweight='bold')
    ax.set_xticks(x_pos)
    ax.set_xticklabels(steps)
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp4_current_vs_peak.png', dpi=150, bbox_inches='tight')
    print(f"   exp4_current_vs_peak.png")
    plt.close()


def plot_experiment5(csv_file, output_dir):
    """
    Experimento 5: Limitação de I/O
    Gráficos:
    - Throughput de escrita vs limite
    - Throughput de leitura vs limite
    - Latência por operação
    - Impacto no tempo total
    """
    print(" Gerando gráficos do Experimento 5 (Limitação de I/O)...")
    
    data = load_csv_data(csv_file)
    
    # Filtrar baseline
    baseline = next((d for d in data if float(d['limit_mbps']) == 0), None)
    limited = [d for d in data if float(d['limit_mbps']) > 0]
    
    limits = [float(d['limit_mbps']) for d in limited]
    write_throughput = [float(d['write_throughput_mbps']) for d in limited]
    read_throughput = [float(d['read_throughput_mbps']) for d in limited]
    write_latency = [float(d['write_latency_ms']) for d in limited]
    read_latency = [float(d['read_latency_ms']) for d in limited]
    total_times = [float(d['total_time_sec']) for d in limited]
    
    baseline_write = float(baseline['write_throughput_mbps']) if baseline else max(write_throughput)
    baseline_read = float(baseline['read_throughput_mbps']) if baseline else max(read_throughput)
    baseline_time = float(baseline['total_time_sec']) if baseline else min(total_times)
    
    # Gráfico 1: Throughput de Escrita
    plt.figure(figsize=(10, 6))
    plt.plot(limits, write_throughput, marker='o', linewidth=2, markersize=10, color='royalblue', label='Medido')
    plt.plot(limits, limits, 'r--', linewidth=2, label='Limite Configurado')
    plt.axhline(y=baseline_write, color='green', linestyle='--', alpha=0.7, label=f'Baseline ({baseline_write:.0f} MB/s)')
    plt.xlabel('Limite Configurado (MB/s)', fontsize=12)
    plt.ylabel('Throughput de Escrita (MB/s)', fontsize=12)
    plt.title('Throughput de Escrita vs Limite', fontsize=14, fontweight='bold')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp5_write_throughput.png', dpi=150, bbox_inches='tight')
    print(f"   exp5_write_throughput.png")
    plt.close()
    
    # Gráfico 2: Throughput de Leitura
    plt.figure(figsize=(10, 6))
    plt.plot(limits, read_throughput, marker='s', linewidth=2, markersize=10, color='crimson', label='Medido')
    plt.axhline(y=baseline_read, color='green', linestyle='--', alpha=0.7, label=f'Baseline ({baseline_read:.0f} MB/s)')
    plt.xlabel('Limite Configurado (MB/s)', fontsize=12)
    plt.ylabel('Throughput de Leitura (MB/s)', fontsize=12)
    plt.title('Throughput de Leitura vs Limite', fontsize=14, fontweight='bold')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp5_read_throughput.png', dpi=150, bbox_inches='tight')
    print(f"   exp5_read_throughput.png")
    plt.close()
    
    # Gráfico 3: Latências
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    ax1.bar(range(len(limits)), write_latency, color='steelblue', alpha=0.7, edgecolor='black')
    ax1.set_xticks(range(len(limits)))
    ax1.set_xticklabels([f'{int(l)} MB/s' for l in limits])
    ax1.set_title('Latência de Escrita', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Limite', fontsize=12)
    ax1.set_ylabel('Latência (ms/MB)', fontsize=12)
    ax1.grid(True, alpha=0.3, axis='y')
    
    ax2.bar(range(len(limits)), read_latency, color='coral', alpha=0.7, edgecolor='black')
    ax2.set_xticks(range(len(limits)))
    ax2.set_xticklabels([f'{int(l)} MB/s' for l in limits])
    ax2.set_title('Latência de Leitura', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Limite', fontsize=12)
    ax2.set_ylabel('Latência (ms/MB)', fontsize=12)
    ax2.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp5_latency.png', dpi=150, bbox_inches='tight')
    print(f"   exp5_latency.png")
    plt.close()
    
    # Gráfico 4: Impacto no Tempo Total
    labels = ['Baseline'] + [f'{int(l)} MB/s' for l in limits]
    times = [baseline_time] + total_times
    
    plt.figure(figsize=(10, 6))
    colors = ['green'] + ['steelblue'] * len(limits)
    bars = plt.bar(range(len(labels)), times, color=colors, alpha=0.7, edgecolor='black')
    plt.xticks(range(len(labels)), labels, rotation=15)
    plt.title('Impacto no Tempo Total de Execução', fontsize=14, fontweight='bold')
    plt.xlabel('Configuração', fontsize=12)
    plt.ylabel('Tempo Total (segundos)', fontsize=12)
    plt.grid(True, alpha=0.3, axis='y')
    
    # Adicionar valores e % de aumento
    for i, (bar, time) in enumerate(zip(bars, times)):
        plt.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                f'{time:.2f}s', ha='center', va='bottom', fontsize=9)
        if i > 0:
            increase = ((time - baseline_time) / baseline_time) * 100
            plt.text(bar.get_x() + bar.get_width()/2., bar.get_height() * 0.5,
                    f'{increase:+.1f}%', ha='center', va='center', fontsize=8, 
                    fontweight='bold', color='white')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/exp5_time_impact.png', dpi=150, bbox_inches='tight')
    print(f"   exp5_time_impact.png")
    plt.close()


def generate_all_experiments(output_dir):
    """Gera visualizações para todos os experimentos encontrados"""
    experiments = {
        'experiment1_overhead.csv': plot_experiment1,
        'experiments/exp2_namespace_isolation.json': plot_experiment2,
        'experiment3_cpu_throttling.csv': plot_experiment3,
        'experiment4_memory_limit.csv': plot_experiment4,
        'experiment5_io_limit.csv': plot_experiment5,
    }
    
    found_count = 0
    for filename, plot_func in experiments.items():
        filepath = f'output/{filename}'
        if os.path.exists(filepath):
            print(f"\n Processando {filename}...")
            try:
                plot_func(filepath, output_dir)
                found_count += 1
            except Exception as e:
                print(f"   Erro ao processar {filename}: {e}")
        else:
            print(f"   Arquivo não encontrado: {filepath}")
    
    if found_count == 0:
        print("\n Nenhum arquivo de experimento encontrado em output/")
        print("Execute os experimentos primeiro com: sudo ./bin/monitor experiment <1-5>")
    else:
        print(f"\n {found_count} experimento(s) visualizado(s) com sucesso!")


if __name__ == "__main__":
    main()
