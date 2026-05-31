import os

def center_obj_file(input_path, output_path):
    print(f"Naprawiam plik: {input_path}...")
    
    vertices = []
    lines = []
    
    # 1. Wczytanie pliku
    with open(input_path, 'r') as file:
        for line in file:
            lines.append(line)
            if line.startswith('v '):
                parts = line.split()
                vertices.append([float(parts[1]), float(parts[2]), float(parts[3])])

    if not vertices:
        print("Błąd: Nie znaleziono wierzchołków w pliku!")
        return

    # 2. Obliczenie granic modelu (Bounding Box)
    min_x = min(v[0] for v in vertices)
    max_x = max(v[0] for v in vertices)
    min_y = min(v[1] for v in vertices)
    max_y = max(v[1] for v in vertices)
    min_z = min(v[2] for v in vertices)
    max_z = max(v[2] for v in vertices)

    # 3. POPRAWIONE: Obliczenie idealnego środka
    center_x = (min_x + max_x) / 2.0
    center_y = (min_y + max_y) / 2.0 # Głębokość wyśrodkowana
    center_z = min_z # <-- KLUCZ: Wysokość Z ustawiona idealnie na najniższy punkt podstawy!

    # 4. Zapisanie skorygowanego pliku
    v_index = 0
    with open(output_path, 'w') as file:
        for line in lines:
            if line.startswith('v '):
                new_x = vertices[v_index][0] - center_x
                new_y = vertices[v_index][1] - center_y
                new_z = vertices[v_index][2] - center_z
                file.write(f"v {new_x:.6f} {new_y:.6f} {new_z:.6f}\n")
                v_index += 1
            else:
                file.write(line)

    print(f"Sukces! Nowy model zapisany jako: {output_path}")

# Uruchom ten sam skrypt dla wszystkich figur:
center_obj_file("Pawn.obj", "Pawn_fixed.obj")
# center_obj_file("Rook.obj", "Rook_fixed.obj")
# center_obj_file("Knight.obj", "Knight_fixed.obj")
# center_obj_file("Bishop.obj", "Bishop_fixed.obj")
# center_obj_file("Queen.obj", "Queen_fixed.obj")
# center_obj_file("King.obj", "King_fixed.obj")