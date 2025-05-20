# DOOM - Grupo 11 - T2

Este es un prototipo de juego de disparos en primera persona (FPS) desarrollado como parte del Trabajo Práctico 1 del Grupo 11. El proyecto está construido utilizando OpenGL y GLUT para la renderización gráfica y la gestión de la ventana.

## Descripción

El prototipo busca implementar mecánicas básicas de un FPS, incluyendo:

* **Movimiento del jugador:** Permite al jugador moverse hacia adelante, hacia atrás, hacia la izquierda y hacia la derecha, así como saltar.
* **Cámara en primera persona:** La vista del jugador se controla mediante el movimiento del ratón, permitiendo mirar alrededor en un entorno 3D.
* **Entorno 3D básico:** Se incluye un entorno simple con un suelo, paredes y un techo para la navegación.
* **Enemigos:** Se han implementado enemigos básicos (cubos rojos) que pueden colisionar con el jugador. La colisión reduce las vidas del jugador.
* **HUD (Heads-Up Display):** Se muestra información esencial en pantalla, como la cantidad de vidas y munición (simulada). También incluye una representación visual del rostro del Doomguy.
* **Disparo:** El jugador puede disparar (tecla 'f'). Actualmente, el disparo implementa una detección básica de colisión con los enemigos mediante un rayo.
* **Animación de disparo:** Al disparar, se reproduce una animación básica del arma.
* **Texturas:** Se han aplicado texturas al suelo y a la cara del Doomguy para mejorar la apariencia visual.
* **Pantalla de inicio:** Se implementa una pantalla de inicio básica que se muestra al iniciar el juego.
* **Minimapa:** Se incluye un minimapa en la esquina superior derecha para ayudar a la navegación, mostrando la posición del jugador y los enemigos.
* **Estado de juego:** Se implementa un estado de "juego terminado" cuando el jugador se queda sin vidas.

## Controles

* **W:** Mover hacia adelante
* **S:** Mover hacia atrás
* **A:** Mover hacia la izquierda
* **D:** Mover hacia la derecha
* **Espacio:** Saltar
* **F:** Disparar
* **Movimiento del ratón:** Controlar la vista de la cámara
* **ESC:** Salir del juego

## Dependencias

* OpenGL
* GLUT (OpenGL Utility Toolkit)
* STB Image (para la carga de texturas)
* [Potencialmente libgif si lograste integrarlo para la animación del arma]

## Instrucciones de Compilación (Dev-C++)

1.  Asegúrate de tener Dev-C++ instalado.
2.  Abre el archivo del proyecto (`.dev`).
3.  Verifica que las bibliotecas GLUT y OpenGL estén correctamente enlazadas en la configuración del proyecto.
4.  Compila el proyecto (menú "Ejecutar" -> "Compilar").
5.  Ejecuta el programa (menú "Ejecutar" -> "Ejecutar").

## Créditos

Este proyecto fue desarrollado por el Grupo 11 como parte de un trabajo práctico.

## Notas Futuras (Posibles mejoras)

* Implementación completa de la animación del arma con carga de GIFs.
* Mejora de la IA de los enemigos.
* Implementación de diferentes tipos de enemigos.
* Diseño de niveles más complejos.
* Implementación de un sistema de puntuación.
* Efectos de sonido.
