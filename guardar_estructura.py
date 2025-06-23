import os

def guardar_estructura(directorio_raiz, archivo_salida="estructura.txt"):
    with open(archivo_salida, "w", encoding="utf-8") as f:
        for raiz, carpetas, archivos in os.walk(directorio_raiz):
            # Filter out .git and venvparalela folders
            carpetas[:] = [carpeta for carpeta in carpetas if carpeta not in [".git", "venvparalela"]]
            nivel = raiz.replace(directorio_raiz, '').count(os.sep)
            sangria = '│   ' * nivel
            f.write(f"{sangria}├── {os.path.basename(raiz)}/\n")
            for archivo in archivos:
                # Skip .gitignore file
                if archivo != ".gitignore":
                    f.write(f"{sangria}│   └── {archivo}\n")

if __name__ == "__main__":
    guardar_estructura(os.getcwd())  # o reemplaza con el path raíz que desees
    print("✅ Estructura de carpetas guardada en 'estructura.txt'")
