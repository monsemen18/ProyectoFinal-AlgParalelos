import subprocess
import re
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# USO: python3 benchmark.py

# --- CONFIGURACIÓN ---
escenas = ['scene_baja.json', 'scene_media.json', 'scene_alta.json']
nombres_tamano = ['Baja', 'Media', 'Alta']
hilos_a_probar = [2, 4, 8, 16]
ejecuciones = 5
ejecutable = './raytracer'

resultados = []

def ejecutar_y_extraer_tiempo(comando):
    tiempos = []
    for i in range(ejecuciones):
        # Ejecutamos el programa compilado en C
        result = subprocess.run(comando, capture_output=True, text=True)
        
        match = re.search(r'Tiempo:\s*([\d.]+)\s*s', result.stdout)
        if match:
            tiempos.append(float(match.group(1)))
        else:
            print(f"Error extrayendo tiempo. Salida: {result.stdout}")
            
    # Retornamos el promedio y la desviación estándar
    return np.mean(tiempos), np.std(tiempos)

# --- EJECUCIÓN DE PRUEBAS ---
for i, escena in enumerate(escenas):
    tamano = nombres_tamano[i]
    print(f"\n--- Procesando escena: {tamano} ---")

    # 1. Ejecución Secuencial 
    print("  Ejecutando versión Secuencial...")
    comando_serial = [ejecutable, '-s', escena, '--serial']
    t_serial_mean, t_serial_std = ejecutar_y_extraer_tiempo(comando_serial)

    # Guardamos los datos 
    resultados.append({
        'Tamaño': tamano,
        'Hilos (p)': 1,
        'Tiempo (s)': round(t_serial_mean, 4),
        'Desv. estándar': round(t_serial_std, 4),
        'Speedup': 1.0,
        'Eficiencia (%)': 100.0
    })

    # 2. Ejecución Paralela
    for p in hilos_a_probar:
        print(f"  Ejecutando versión Paralela con {p} hilos...")
        comando_paralelo = [ejecutable, '-s', escena, '-t', str(p)]
        t_par_mean, t_par_std = ejecutar_y_extraer_tiempo(comando_paralelo)

        # Fórmulas de evaluación
        speedup = t_serial_mean / t_par_mean
        eficiencia = (speedup / p) * 100

        resultados.append({
            'Tamaño': tamano,
            'Hilos (p)': p,
            'Tiempo (s)': round(t_par_mean, 4),
            'Desv. estándar': round(t_par_std, 4),
            'Speedup': round(speedup, 4),
            'Eficiencia (%)': round(eficiencia, 2)
        })

# --- GENERACIÓN DE LA TABLA ---
df = pd.DataFrame(resultados)
df.to_csv('resultados_raytracing.csv', index=False)
print("\n¡Pruebas terminadas! Datos guardados en 'resultados_raytracing.csv'.")
print("\nVista previa de la tabla:")
print(df.to_string(index=False))

# --- GENERACIÓN DE GRÁFICAS ---
todos_los_hilos = [1] + hilos_a_probar
colores = {'Baja': 'green', 'Media': 'blue', 'Alta': 'red'}

# Gráfica 1: Tiempo vs Número de Hilos
plt.figure(figsize=(8, 5))
for tamano in nombres_tamano:
    datos = df[df['Tamaño'] == tamano]
    plt.plot(datos['Hilos (p)'], datos['Tiempo (s)'], marker='o', 
             color=colores[tamano], label=f'Tamaño {tamano}')

plt.xlabel('Número de Hilos (p)')
plt.ylabel('Tiempo de ejecución (s)')
plt.title('Tiempo de Ejecución vs Número de Hilos')
plt.xticks(todos_los_hilos)
plt.legend()
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig('grafica_tiempo.png', dpi=300)

# Gráfica 2: Speedup vs Número de Hilos
plt.figure(figsize=(8, 5))
for tamano in nombres_tamano:
    datos = df[df['Tamaño'] == tamano]
    plt.plot(datos['Hilos (p)'], datos['Speedup'], marker='o', 
             color=colores[tamano], label=f'Tamaño {tamano}')

# la curva ideal
plt.plot(todos_los_hilos, todos_los_hilos, color='black', linestyle='--', label='Curva ideal S(p) = p')

plt.xlabel('Número de Hilos (p)')
plt.ylabel('Speedup (Aceleración)')
plt.title('Speedup vs Número de Hilos')
plt.xticks(todos_los_hilos)
plt.legend()
plt.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig('grafica_speedup.png', dpi=300)

print("Gráficas: 'grafica_tiempo.png' y 'grafica_speedup.png'.")