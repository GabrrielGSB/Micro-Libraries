# servidor_dashboard.py
from flask import Flask, send_file, jsonify, request
import json
import os
from pathlib import Path
from datetime import datetime

app = Flask(__name__)

DATA_DIR = "./data"
Path(DATA_DIR).mkdir(exist_ok=True)

def find_latest_jsonl(directory=DATA_DIR):
    files = list(Path(directory).glob("memory_*.jsonl"))
    if not files:
        return None
    return str(max(files, key=os.path.getmtime))

JSONL_FILE = find_latest_jsonl() or os.path.join(DATA_DIR, "current.jsonl")

snapshots_cache = []
last_mtime = 0

def load_snapshots():
    global snapshots_cache, last_mtime
    try:
        mtime = os.path.getmtime(JSONL_FILE)
        if mtime == last_mtime:
            return
        last_mtime = mtime
        print(f"[SERVIDOR] Recarregando {JSONL_FILE} (modificado)")
    except OSError:
        snapshots_cache = []
        return

    snapshots = []
    with open(JSONL_FILE, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                snapshots.append(json.loads(line))
            except json.JSONDecodeError:
                continue
    snapshots_cache = snapshots
    print(f"[SERVIDOR] Cache atualizado: {len(snapshots_cache)} snapshots")

def calculate_variations(snapshots):
    variations = []
    for i in range(1, len(snapshots)):
        prev = snapshots[i-1]
        curr = snapshots[i]
        variation = {
            'teste': curr.get('teste', '?'),
            'sram_livre': curr.get('sram_livre'),
            'dram_livre': curr.get('dram_livre'),
            'iram_livre': curr.get('iram_livre'),
            'maior_bloco_livre': curr.get('maior_bloco_livre'),
            'delta_sram': curr.get('sram_livre', 0) - prev.get('sram_livre', 0),
            'delta_dram': curr.get('dram_livre', 0) - prev.get('dram_livre', 0),
            'delta_iram': curr.get('iram_livre', 0) - prev.get('iram_livre', 0),
            'delta_maior_bloco': curr.get('maior_bloco_livre', 0) - prev.get('maior_bloco_livre', 0),
        }
        variations.append(variation)
    return variations

@app.route('/')
def index():
    return send_file('dashboard.html')

@app.route('/api/memory')
def memory_data():
    load_snapshots()
    if not snapshots_cache:
        return jsonify({})
    last = snapshots_cache[-1]
    variation = None
    if len(snapshots_cache) >= 2:
        prev = snapshots_cache[-2]
        variation = {
            'delta_sram': last.get('sram_livre', 0) - prev.get('sram_livre', 0),
            'delta_dram': last.get('dram_livre', 0) - prev.get('dram_livre', 0),
            'delta_iram': last.get('iram_livre', 0) - prev.get('iram_livre', 0),
            'delta_maior_bloco': last.get('maior_bloco_livre', 0) - prev.get('maior_bloco_livre', 0),
        }
    return jsonify({
        'snapshot': last,
        'variation': variation,
        'count': len(snapshots_cache)
    })

@app.route('/api/variations')
def get_variations():
    limit = request.args.get('limit', 5, type=int)
    load_snapshots()
    all_vars = calculate_variations(snapshots_cache)
    recent = all_vars[-limit:] if len(all_vars) > limit else all_vars
    return jsonify(recent)

@app.route('/api/all_variations')
def get_all_variations():
    load_snapshots()
    all_vars = calculate_variations(snapshots_cache)
    return jsonify(all_vars)

@app.route('/api/history')
def get_history():
    load_snapshots()
    history = []
    for snap in snapshots_cache:
        history.append({
            'teste': snap.get('teste'),
            'sram_livre': snap.get('sram_livre'),
            'dram_livre': snap.get('dram_livre'),
            'iram_livre': snap.get('iram_livre'),
            'maior_bloco_livre': snap.get('maior_bloco_livre'),
        })
    return jsonify(history)

# ---------- NOVA LISTAGEM DE ARQUIVOS JSONL NA PASTA DATA ----------
@app.route('/api/saves')
def list_saves():
    """Lista arquivos .jsonl disponíveis na pasta de dados."""
    files = []
    for f in Path(DATA_DIR).glob("memory_*.jsonl"):
        files.append({
            "filename": f.name,
            "size": f.stat().st_size,
            "mtime": f.stat().st_mtime,
        })
    # Mais recentes primeiro
    files.sort(key=lambda x: x['mtime'], reverse=True)
    return jsonify(files)

@app.route('/api/load/<filename>')
def load_saved(filename):
    """Carrega um arquivo .jsonl da pasta data e retorna todos os snapshots."""
    filepath = Path(DATA_DIR) / filename
    if not filepath.exists():
        return jsonify({"error": "Arquivo não encontrado"}), 404
    try:
        snapshots = []
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                snapshots.append(json.loads(line))
        return jsonify(snapshots)
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    print("Servidor iniciado em http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=False)