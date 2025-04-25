#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <math.h>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define M_PI 3.14159265358979323846
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ==== CONSTANTES ====
const float VELOCIDAD_MOVIMIENTO = 0.14f;
const float VELOCIDAD_SALTO_INICIAL = 0.07f;
const float GRAVEDAD = 0.01f;
const float RADIO_JUGADOR = 0.6f;
const float DISTANCIA_COLISION_CUADRADA = RADIO_JUGADOR * RADIO_JUGADOR;

// ==== CÁMARA ====
float angulo_yaw = -90.0f; // inicial mirando hacia -Z
float angulo_pitch = 0.0f; // sin inclinación vertical
float ultimo_mouseX = 400.0f, ultimo_mouseY = 300.0f; // centro de pantalla
bool primer_mouse = true;
bool centrar_cursor = false;

// ==== GLOBALES ====
int ancho_ventana = 1200;
int alto_ventana = 800;
int centro_X = ancho_ventana / 2;
int centro_Y = alto_ventana / 2;
float direccion_camara_x = 0.0f, direccion_camara_y = 0.0f, direccion_camara_z = -1.0f;

// ==== VARIABLES GLOBALES ====
float posicion_camara_x = 0.0f, posicion_camara_y = 1.0f, posicion_camara_z = 0.0f;
float altura_salto = 0.10f, velocidad_salto = 0.10f;
bool esta_saltando = false;

int vidas = 3;
bool juego_terminado = false;
bool juego_iniciado = false; // Nuevo estado para la pantalla de inicio


float rebote_enemigo = 0.1f;
float direccion_rebote = 1.0f;
const int NUM_ENEMIGOS = 5;
float posicion_enemigo[NUM_ENEMIGOS][2] = {
    {-5.0f, 5.0f},
    {5.0f, -5.0f},
    {0.0f, 7.0f},
    {-10.0f, -10.0f}, // Posición del enemigo 4
    {15.0f, 2.0f}      // Posición del enemigo 5
};

int ancho_pantalla = 1200;
int alto_pantalla = 800;

struct Bala {
    float posicion[3];
    float direccion[3];
};

// Declara una variable para la textura 
GLuint texturaID_pared; 
GLuint texturaID_techo; 
GLuint texturaID_suelo;
GLuint texturaID_cara_doomguy;
GLuint texturaID_pistola;
std::vector<GLuint> pistola_animacion_texturas;
int pistola_animacion_cuadro_actual = 0;
bool esta_animando_disparo = false;
int tiempo_inicio_animacion = 0;
int duracion_entre_cuadros = 80; // Milisegundos por cuadro
std::vector<Bala> balas;


void cargarAnimacionPistola(const char* archivo_gif) {
    // --- Lógica simulada para cargar la animación ---
    printf("Cargando animación de pistola desde %s\n", archivo_gif);
    // *** ¡IMPORTANTE! ***
    // Aquí DEBES implementar la lógica real para cargar los cuadros del GIF.
    // Esto requerirá una biblioteca externa para decodificar GIFs (como libgif).
    // El siguiente es un ejemplo SIMULADO para que el resto del código funcione
    // sin errores, pero NO CARGA un GIF real.
    for (int i = 0; i < 5; ++i) {
        GLuint textura_frame = 0; // Simula la carga de un cuadro
        pistola_animacion_texturas.push_back(textura_frame);
    }
    // --- Reemplaza la simulación con tu implementación real de carga de GIF ---
}

//Función para cargar la textura
GLuint cargarTextura(const char* ruta) {
    int ancho, alto, canales;
    unsigned char* data = stbi_load(ruta, &ancho, &alto, &canales, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Error al cargar la textura: " << ruta << std::endl;
        return 0;
    }

    GLuint id_textura;
    glGenTextures(1, &id_textura);
    glBindTexture(GL_TEXTURE_2D, id_textura);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ancho, alto, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return id_textura;
}

// Estructura para representar la caja de colisión de un objeto (enemigo)
struct CajaColision {
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

// Función para obtener la caja de colisión de un enemigo (ajusta según tu representación)
CajaColision obtenerCajaColisionEnemigo(int indice_enemigo) {
    CajaColision caja;
    float tamano = 10.0f; // Asumiendo que tus cubos enemigos tienen lado 2.0, el "radio" es 1.0
    caja.minX = posicion_enemigo[indice_enemigo][0] - tamano;
    caja.maxX = posicion_enemigo[indice_enemigo][0] + tamano;
    caja.minY = 0.0f;     // Ajusta si tus enemigos no están a nivel del suelo
    caja.maxY = 2.0f;     // Ajusta según la altura de tus enemigos
    caja.minZ = posicion_enemigo[indice_enemigo][1] - tamano;
    caja.maxZ = posicion_enemigo[indice_enemigo][1] + tamano;
    return caja;
}

// Función para verificar si un rayo intersecta con una caja de colisión
bool rayoIntersectaCaja(float origenX, float origenY, float origenZ,
                        float direccionX, float direccionY, float direccionZ,
                        const CajaColision& caja, float& t) {
    float tMin = -INFINITY;
    float tMax = INFINITY;

    float limites[] = {caja.minX, caja.maxX, caja.minY, caja.maxY, caja.minZ, caja.maxZ};
    float origenes[] = {origenX, origenY, origenZ};
    float direcciones[] = {direccionX, direccionY, direccionZ};

    for (int i = 0; i < 3; ++i) {
        if (std::abs(direcciones[i]) < 1e-6) { // Rayo paralelo al plano
            if (origenes[i] < limites[2 * i] || origenes[i] > limites[2 * i + 1]) {
                return false;
            }
        } else {
            float t1 = (limites[2 * i] - origenes[i]) / direcciones[i];
            float t2 = (limites[2 * i + 1] - origenes[i]) / direcciones[i];

            if (t1 > t2) std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax) {
                return false;
            }
        }
    }

    t = tMin; // Distancia al punto de intersección
    return true;
}

// ==== UTILITARIAS ====
void dibujarHUD() {
    // Cambiar a modo 2D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Dibujar barra inferior del HUD
    glColor3f(0.1f, 0.1f, 0.1f); // Gris oscuro
    glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(ancho_pantalla, 0);
        glVertex2f(ancho_pantalla, 100);
        glVertex2f(0, 100);
    glEnd();

       // Texto de vidas (más grande)
    char buffer[50];
    sprintf(buffer, "Vidas: %d", vidas);
    glColor3f(1.0f, 0.0f, 0.0f); // Rojo
    glRasterPos2f(30, 75); // Ajusté la posición vertical
    for (char* c = buffer; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c); // Fuente más grande (alternativa)
    }

    // --- CENTRO INFERIOR DEL HUD (Cara de Doomguy - Ajustada) ---
    glColor3f(1.0f, 1.0f, 1.0f); // Establecer color blanco para la textura
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaID_cara_doomguy);

    float ancho_cara = 120;
    float alto_cara = 90;
    float posicion_cara_x = ancho_pantalla / 2 - ancho_cara / 2;
    float posicion_cara_y = 5; // Ajusté la posición vertical

    float sprite_ancho_atlas = 1.0f / 8.0f; // Ancho de cada sprite en el atlas
    float sprite_alto_atlas = 0.8f / 5.35f;  // Alto de cada sprite en el atlas
    float zoom_factor_u = 0.910f; // Factor de "zoom" horizontal (mayor valor = más zoom)
    float zoom_factor_v = 1.05f; // Factor de "zoom" vertical
    float centro_u = 0.0f;      // Desplazamiento horizontal para centrar (0 para la primera cara)
    float centro_v = 0.0f;      // Desplazamiento vertical para centrar (0 para la fila superior)

   glBegin(GL_QUADS);
        glTexCoord2f(centro_u, centro_v + sprite_alto_atlas * zoom_factor_v); glVertex2f(posicion_cara_x, posicion_cara_y);             // Esquina inferior izquierda (V invertida)
        glTexCoord2f(centro_u + sprite_ancho_atlas * zoom_factor_u, centro_v + sprite_alto_atlas * zoom_factor_v); glVertex2f(posicion_cara_x + ancho_cara, posicion_cara_y); // Esquina inferior derecha (V invertida)
        glTexCoord2f(centro_u + sprite_ancho_atlas * zoom_factor_u, centro_v); glVertex2f(posicion_cara_x + ancho_cara, posicion_cara_y + alto_cara);  // Esquina superior derecha (V invertida)
        glTexCoord2f(centro_u, centro_v); glVertex2f(posicion_cara_x, posicion_cara_y + alto_cara);              // Esquina superior izquierda (V invertida)
    glEnd();

    glDisable(GL_TEXTURE_2D);

    // Texto de munición (más grande y ajustado)
    sprintf(buffer, "Municion: %d", 42); // puedes hacerlo variable
    glColor3f(1.0f, 1.0f, 0.0f); // Amarillo
    glRasterPos2f(ancho_pantalla - 180, 75); // Mover más a la derecha y ajustar vertical
    for (char* c = buffer; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c); // Fuente más grande (alternativa)
    }
      

    // Restaurar matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void movimientoMouse(int x, int y) {
    if (centrar_cursor) {
        centrar_cursor = false;
        return;
    }

    if (primer_mouse) {
        ultimo_mouseX = x;
        ultimo_mouseY = y;
        primer_mouse = false;
    }

    float desplazamientoX = x - ultimo_mouseX;
    float desplazamientoY = ultimo_mouseY - y; // y va de arriba a abajo
    ultimo_mouseX = x;
    ultimo_mouseY = y;

    float sensibilidad = 0.1f;
    desplazamientoX *= sensibilidad;
    desplazamientoY *= sensibilidad;

    angulo_yaw += desplazamientoX;
    angulo_pitch += desplazamientoY;

    if (angulo_pitch > 89.0f) angulo_pitch = 89.0f;
    if (angulo_pitch < -89.0f) angulo_pitch = -89.0f;

    float radianes_yaw = angulo_yaw * M_PI / 180.0f;
    float radianes_pitch = angulo_pitch * M_PI / 180.0f;

    direccion_camara_x = cos(radianes_yaw) * cos(radianes_pitch);
    direccion_camara_y = sin(radianes_pitch);
    direccion_camara_z = sin(radianes_yaw) * cos(radianes_pitch);

    // Recentrar cursor para simular "captura"
    centrar_cursor = true;
    glutWarpPointer(centro_X, centro_Y);
    ultimo_mouseX = centro_X;
    ultimo_mouseY = centro_Y;


    glutPostRedisplay();
}

void resetearPosicionJugador() {
    posicion_camara_x = posicion_camara_z = 0.0f;
    altura_salto = 0.0f;
}

bool verificarColision() {
    for (int i = 0; i < 3; ++i) {
        float dx = posicion_camara_x - posicion_enemigo[i][0];
        float dz = posicion_camara_z - posicion_enemigo[i][1];
        float distancia_cuadrada = dx * dx + dz * dz;
        if (distancia_cuadrada < DISTANCIA_COLISION_CUADRADA) return true;
    }
    return false;
}

void dibujarLineaPared(float fijo, bool horizontal) {
    for (float var = -0.0f; var <= 50.0f; var += 2.0f) {
        glPushMatrix();
        if (horizontal)
            glTranslatef(fijo, 1.0f, var);
        else
            glTranslatef(var, 1.0f, fijo);
        glutSolidCube(2.0f);
        glPopMatrix();
    }
}

// Función auxiliar para dibujar un cubo
void dibujarCubo(float x, float y, float z, float ancho, float alto, float profundidad) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(ancho, alto, profundidad);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void dibujarTexto(float x, float y, const char* texto) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glRasterPos2f(x, y);
    for (const char* c = texto; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void dibujarMinimapa(float grosor_pared, float altura_pared) {
    // Cambiar a modo 2D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Definir el tamaño y la posición del minimapa
    float tamano_mapa = 200.0f;
    float margen_derecho = 30.0f;
    float margen_superior = 30.0f;
    float posicion_mapa_x = ancho_pantalla - tamano_mapa - margen_derecho;
    float posicion_mapa_y = alto_pantalla - tamano_mapa - margen_superior;

    // Dibujar el fondo del minimapa con un borde
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
        glVertex2f(posicion_mapa_x - 2.0f, posicion_mapa_y - 2.0f);
        glVertex2f(posicion_mapa_x + tamano_mapa + 2.0f, posicion_mapa_y - 2.0f);
        glVertex2f(posicion_mapa_x + tamano_mapa + 2.0f, posicion_mapa_y + tamano_mapa + 2.0f);
        glVertex2f(posicion_mapa_x - 2.0f, posicion_mapa_y + tamano_mapa + 2.0f);
    glEnd();
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2f(posicion_mapa_x, posicion_mapa_y);
        glVertex2f(posicion_mapa_x + tamano_mapa, posicion_mapa_y);
        glVertex2f(posicion_mapa_x + tamano_mapa, posicion_mapa_y + tamano_mapa);
        glVertex2f(posicion_mapa_x, posicion_mapa_y + tamano_mapa);
    glEnd();

    // Escalar las coordenadas del mundo al espacio del minimapa
    float escala_mapa = tamano_mapa / 100.0f; // Asumiendo que tu mundo tiene ~100x100 unidades
    float offset_mapa_x = posicion_mapa_x + tamano_mapa / 2.0f;
    float offset_mapa_y = posicion_mapa_y + tamano_mapa / 2.0f;
    float offset_mundo = 0.0f; // Si tu laberinto no está centrado en (0,0), ajusta esto

    // Dibujar las paredes del laberinto en el minimapa como líneas blancas
    glColor3f(1.0f, 1.0f, 1.0f);
    float grosor_pared_mapa = grosor_pared * escala_mapa;
    float altura_pared_mapa = altura_pared * escala_mapa; // Aunque la altura no es tan relevante en 2D

    for (float i = -50.0f; i <= 50.0f; i += grosor_pared) {
        // Pared Norte
        glBegin(GL_LINES);
            glVertex2f(offset_mapa_x + (i + offset_mundo) * escala_mapa, offset_mapa_y + (-50.0f + offset_mundo) * escala_mapa);
            glVertex2f(offset_mapa_x + (i + grosor_pared + offset_mundo) * escala_mapa, offset_mapa_y + (-50.0f + offset_mundo) * escala_mapa);
        glEnd();
        // Pared Sur
        glBegin(GL_LINES);
            glVertex2f(offset_mapa_x + (i + offset_mundo) * escala_mapa, offset_mapa_y + (50.0f + offset_mundo) * escala_mapa);
            glVertex2f(offset_mapa_x + (i + grosor_pared + offset_mundo) * escala_mapa, offset_mapa_y + (50.0f + offset_mundo) * escala_mapa);
        glEnd();
        // Pared Oeste
        glBegin(GL_LINES);
            glVertex2f(offset_mapa_x + (-50.0f + offset_mundo) * escala_mapa, offset_mapa_y + (i + offset_mundo) * escala_mapa);
            glVertex2f(offset_mapa_x + (-50.0f + offset_mundo) * escala_mapa, offset_mapa_y + (i + grosor_pared + offset_mundo) * escala_mapa);
        glEnd();
        // Pared Este
        glBegin(GL_LINES);
            glVertex2f(offset_mapa_x + (50.0f + offset_mundo) * escala_mapa, offset_mapa_y + (i + offset_mundo) * escala_mapa);
            glVertex2f(offset_mapa_x + (50.0f + offset_mundo) * escala_mapa, offset_mapa_y + (i + grosor_pared + offset_mundo) * escala_mapa);
        glEnd();
    }

    // Dibujar las paredes interiores (basadas en tu último diseño)
    glColor3f(1.0f, 1.0f, 1.0f);
    auto dibujar_pared_mapa = [&](float x1, float z1, float x2, float z2) {
        glBegin(GL_LINES);
            glVertex2f(offset_mapa_x + (x1 + offset_mundo) * escala_mapa, offset_mapa_y + (z1 + offset_mundo) * escala_mapa);
            glVertex2f(offset_mapa_x + (x2 + offset_mundo) * escala_mapa, offset_mapa_y + (z2 + offset_mundo) * escala_mapa);
        glEnd();
    };

    // Pared vertical izquierda
    for (float z = -40.0f; z <= 40.0f; z += 10.0f) {
        dibujar_pared_mapa(-40.0f, z, -40.0f, z + grosor_pared);
    }
    // Paredes horizontales desde la izquierda
    dibujar_pared_mapa(-40.0f, 30.0f, -40.0f + grosor_pared * 5, 30.0f);
    dibujar_pared_mapa(-40.0f, 10.0f, -40.0f + grosor_pared * 3, 10.0f);
    dibujar_pared_mapa(-40.0f, -10.0f, -40.0f + grosor_pared * 4, -10.0f);
    dibujar_pared_mapa(-40.0f, -30.0f, -40.0f + grosor_pared * 2, -30.0f);
    // Pared vertical de conexión
    for (float y = -20.0f; y <= 20.0f; y += 10.0f) {
        dibujar_pared_mapa(-20.0f, y, -20.0f, y + grosor_pared);
    }
    // Pared horizontal inferior derecha
    dibujar_pared_mapa(-10.0f, -40.0f, -10.0f + grosor_pared * 6, -40.0f);

    // Dibujar al jugador como un punto amarillo
    glColor3f(1.0f, 1.0f, 0.0f);
    float jugador_mapa_x = offset_mapa_x + posicion_camara_x * escala_mapa;
    float jugador_mapa_y = offset_mapa_y + posicion_camara_z * escala_mapa;
    float tamano_jugador = 6.0f;
    glBegin(GL_QUADS);
        glVertex2f(jugador_mapa_x - tamano_jugador / 2, jugador_mapa_y - tamano_jugador / 2);
        glVertex2f(jugador_mapa_x + tamano_jugador / 2, jugador_mapa_y - tamano_jugador / 2);
        glVertex2f(jugador_mapa_x + tamano_jugador / 2, jugador_mapa_y + tamano_jugador / 2);
        glVertex2f(jugador_mapa_x - tamano_jugador / 2, jugador_mapa_y + tamano_jugador / 2);
    glEnd();

	// Dibujar a los enemigos como puntos rojos
	glColor3f(1.0f, 0.0f, 0.0f);
	float tamano_enemigo = 6.0f;
	for (int i = 0; i < NUM_ENEMIGOS; ++i) {
	    float enemigo_mapa_x = offset_mapa_x + posicion_enemigo[i][0] * escala_mapa;
	    float enemigo_mapa_y = offset_mapa_y + posicion_enemigo[i][1] * escala_mapa;
	    glBegin(GL_QUADS);
	        glVertex2f(enemigo_mapa_x - tamano_enemigo / 2, enemigo_mapa_y - tamano_enemigo / 2);
	        glVertex2f(enemigo_mapa_x + tamano_enemigo / 2, enemigo_mapa_y - tamano_enemigo / 2);
	        glVertex2f(enemigo_mapa_x + tamano_enemigo / 2, enemigo_mapa_y + tamano_enemigo / 2);
	        glVertex2f(enemigo_mapa_x - tamano_enemigo / 2, enemigo_mapa_y + tamano_enemigo / 2);
	    glEnd();
	}

    // Restaurar matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void dibujarEscena() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if (!juego_iniciado) {
        // --- PANTALLA DE INICIO ---
        // Deshabilitar la prueba de profundidad para dibujar en 2D
        glDisable(GL_DEPTH_TEST);

        // Dibujar la pantalla de inicio en modo ortogonal 2D
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Dibujar el fondo con degradado vertical
        glBegin(GL_QUADS);
            glColor3f(0.2f, 0.1f, 0.0f); glVertex2f(0, 0);
            glColor3f(0.4f, 0.2f, 0.0f); glVertex2f(ancho_pantalla, 0);
            glColor3f(0.2f, 0.1f, 0.0f); glVertex2f(ancho_pantalla, alto_pantalla);
            glColor3f(0.4f, 0.2f, 0.0f); glVertex2f(0, alto_pantalla);
        glEnd();

        // Dibujar el título "DOOM - GRUPO 11 - T1"
        glColor3f(1.0f, 0.8f, 0.0f); // Amarillo dorado
        const char* texto_titulo = "DOOM - GRUPO 11 - T1";
        int ancho_texto_titulo = 0;
        for (const char* c = texto_titulo; *c; ++c) {
            ancho_texto_titulo += glutBitmapWidth(GLUT_BITMAP_9_BY_15, *c);
        }
        float posicion_texto_x = ancho_pantalla / 2 - ancho_texto_titulo / 2;
        float posicion_texto_y = alto_pantalla / 2 + 30;
        glRasterPos2f(posicion_texto_x, posicion_texto_y);
        for (const char* c = texto_titulo; *c; ++c) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
        }

        // Dibujar la instrucción "PULSA ESPACIO PARA JUGAR"
        glColor3f(0.8f, 0.6f, 0.0f); // Amarillo más apagado
        const char* texto_instruccion = "PULSA ESPACIO PARA JUGAR";
        int ancho_texto_instruccion = 0;
        for (const char* c = texto_instruccion; *c; ++c) {
            ancho_texto_instruccion += glutBitmapWidth(GLUT_BITMAP_8_BY_13, *c);
        }
        float posicion_instruccion_x = ancho_pantalla / 2 - ancho_texto_instruccion / 2;
        float posicion_instruccion_y = alto_pantalla / 2 - 30;
        glRasterPos2f(posicion_instruccion_x, posicion_instruccion_y);
        for (const char* c = texto_instruccion; *c; ++c) {
            glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
        }

        // Restaurar matrices de la pantalla de inicio
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

    } else {
        // --- ESCENA DEL JUEGO: LABERINTO DOOM ---
        glEnable(GL_DEPTH_TEST); // Volver a habilitar la prueba de profundidad para la escena 3D
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPerspective(45.0, (double)ancho_pantalla / (double)alto_pantalla, 1.0, 200.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // --- POSICIÓN DE LA CÁMARA ---
        gluLookAt(posicion_camara_x, posicion_camara_y + altura_salto + 2.0f, posicion_camara_z, 
                  posicion_camara_x + direccion_camara_x, posicion_camara_y + altura_salto + 1.0f + direccion_camara_y, posicion_camara_z + direccion_camara_z,
                  0.0f, 1.0f, 0.0f);

        // --- DIBUJAR EL SUELO ---
        glColor3f(1.0f, 1.0f, 1.0f); // Establecer color blanco para la textura
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturaID_suelo);

        float repetirX = 10.0f; // Repetir 10 veces a lo largo del eje X
        float repetirZ = 10.0f; // Repetir 10 veces a lo largo del eje Z

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-50.0f, 0.0f, -50.0f);
            glTexCoord2f(0.0f, repetirZ); glVertex3f(-50.0f, 0.0f, 50.0f);
            glTexCoord2f(repetirX, repetirZ); glVertex3f(50.0f, 0.0f, 50.0f);
            glTexCoord2f(repetirX, 0.0f); glVertex3f(50.0f, 0.0f, -50.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);

		// --- DIBUJAR EL TECHO ---
        glColor3f(1.0f, 1.0f, 1.0f); // Color blanco para la textura
        glEnable(GL_TEXTURE_2D);
        texturaID_techo = cargarTextura("techoInfierno.tga"); // Carga la textura
        glBindTexture(GL_TEXTURE_2D, texturaID_techo); // Enlaza la textura

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-50.0f, 5.0f, -50.0f);
            glTexCoord2f(10.0f, 0.0f); glVertex3f(50.0f, 5.0f, -50.0f);
            glTexCoord2f(10.0f, 10.0f); glVertex3f(50.0f, 5.0f, 50.0f);
            glTexCoord2f(0.0f, 10.0f); glVertex3f(-50.0f, 5.0f, 50.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        // --- DIBUJAR LAS PAREDES DEL LABERINTO ---
        glColor3f(0.5f, 0.5f, 0.5f);
        float altura_pared = 4.70f; // Altura de las paredes
        float grosor_pared = 3.0f;

        // Paredes exteriores
        glColor3f(1.0f, 1.0f, 1.0f); // Color blanco para la textura
        glEnable(GL_TEXTURE_2D);
        texturaID_pared = cargarTextura("wall.tga"); // Carga la textura
        glBindTexture(GL_TEXTURE_2D, texturaID_pared); // Enlaza la textura

     	for (float i = -50.0f; i <= 50.0f; i += grosor_pared) {
            // Pared Norte
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(i, 0.0f, -50.0f); // Invertido v
                glTexCoord2f(1.0f, 1.0f); glVertex3f(i + grosor_pared, 0.0f, -50.0f); // Invertido v
                glTexCoord2f(1.0f, 0.0f); glVertex3f(i + grosor_pared, altura_pared, -50.0f);
                glTexCoord2f(0.0f, 0.0f); glVertex3f(i, altura_pared, -50.0f);
            glEnd();

            // Pared Sur
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(i, 0.0f, 50.0f); // Invertido v
                glTexCoord2f(1.0f, 1.0f); glVertex3f(i + grosor_pared, 0.0f, 50.0f); // Invertido v
                glTexCoord2f(1.0f, 0.0f); glVertex3f(i + grosor_pared, altura_pared, 50.0f);
                glTexCoord2f(0.0f, 0.0f); glVertex3f(i, altura_pared, 50.0f);
            glEnd();

            // Pared Oeste
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(-50.0f, 0.0f, i); // Invertido v
                glTexCoord2f(1.0f, 1.0f); glVertex3f(-50.0f, 0.0f, i + grosor_pared); // Invertido v
                glTexCoord2f(1.0f, 0.0f); glVertex3f(-50.0f, altura_pared, i + grosor_pared);
                glTexCoord2f(0.0f, 0.0f); glVertex3f(-50.0f, altura_pared, i);
            glEnd();

            // Pared Este
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(50.0f, 0.0f, i); // Invertido v
                glTexCoord2f(1.0f, 1.0f); glVertex3f(50.0f, 0.0f, i + grosor_pared); // Invertido v
                glTexCoord2f(1.0f, 0.0f); glVertex3f(50.0f, altura_pared, i + grosor_pared);
                glTexCoord2f(0.0f, 0.0f); glVertex3f(50.0f, altura_pared, i);
            glEnd();
        }

        glDisable(GL_TEXTURE_2D);

       	
       	// --- DIBUJAR LAS PAREDES DEL LABERINTO (INTERIORES SEGÚN BOCETO) ---
        glColor3f(1.0f, 1.0f, 1.0f); // Color blanco para la textura
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texturaID_pared);


        // Pared vertical cerca del lado izquierdo
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, -40.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, -40.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, -40.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, -40.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, -30.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, -30.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, -30.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, -30.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, -20.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, -20.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, -20.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, -20.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, -10.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, -10.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, -10.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, -10.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, 0.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, 0.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, 0.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, 0.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, 10.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, 10.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, 10.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, 10.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, 20.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, 20.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, 20.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, 20.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, 30.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, 30.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, 30.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, 30.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, 40.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-40.0f + grosor_pared, 0.0f, 40.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-40.0f + grosor_pared, altura_pared, 40.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, 40.0f); glEnd();

        // Paredes horizontales que se extienden desde la pared izquierda
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, 30.0f); glTexCoord2f(5.0f, 1.0f); glVertex3f(-40.0f + grosor_pared * 5, 0.0f, 30.0f); glTexCoord2f(5.0f, 0.0f); glVertex3f(-40.0f + grosor_pared * 5, altura_pared, 30.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, 30.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, 10.0f); glTexCoord2f(3.0f, 1.0f); glVertex3f(-40.0f + grosor_pared * 3, 0.0f, 10.0f); glTexCoord2f(3.0f, 0.0f); glVertex3f(-40.0f + grosor_pared * 3, altura_pared, 10.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, 10.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, -10.0f); glTexCoord2f(4.0f, 1.0f); glVertex3f(-40.0f + grosor_pared * 4, 0.0f, -10.0f); glTexCoord2f(4.0f, 0.0f); glVertex3f(-40.0f + grosor_pared * 4, altura_pared, -10.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, -10.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-40.0f, 0.0f, -30.0f); glTexCoord2f(2.0f, 1.0f); glVertex3f(-40.0f + grosor_pared * 2, 0.0f, -30.0f); glTexCoord2f(2.0f, 0.0f); glVertex3f(-40.0f + grosor_pared * 2, altura_pared, -30.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-40.0f, altura_pared, -30.0f); glEnd();

        // Pared vertical que conecta algunas horizontales
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-20.0f, 0.0f, -20.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-20.0f + grosor_pared, 0.0f, -20.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-20.0f + grosor_pared, altura_pared, -20.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-20.0f, altura_pared, -20.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-20.0f, 0.0f, -10.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-20.0f + grosor_pared, 0.0f, -10.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-20.0f + grosor_pared, altura_pared, -10.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-20.0f, altura_pared, -10.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-20.0f, 0.0f, 0.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-20.0f + grosor_pared, 0.0f, 0.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-20.0f + grosor_pared, altura_pared, 0.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-20.0f, altura_pared, 0.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-20.0f, 0.0f, 10.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-20.0f + grosor_pared, 0.0f, 10.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-20.0f + grosor_pared, altura_pared, 10.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-20.0f, altura_pared, 10.0f); glEnd();
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-20.0f, 0.0f, 20.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-20.0f + grosor_pared, 0.0f, 20.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-20.0f + grosor_pared, altura_pared, 20.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-20.0f, altura_pared, 20.0f); glEnd();

        // Pared horizontal en la parte inferior derecha
        glBegin(GL_QUADS); glTexCoord2f(0.0f, 1.0f); glVertex3f(-10.0f, 0.0f, -40.0f); glTexCoord2f(6.0f, 1.0f); glVertex3f(-10.0f + grosor_pared * 6, 0.0f, -40.0f); glTexCoord2f(6.0f, 0.0f); glVertex3f(-10.0f + grosor_pared * 6, altura_pared, -40.0f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-10.0f, altura_pared, -40.0f); glEnd();

        glDisable(GL_TEXTURE_2D);
       	
       	
       	
		// --- DIBUJAR LOS ENEMIGOS ---
		    glColor3f(1.0f, 0.0f, 0.0f);
		    for (int i = 0; i < NUM_ENEMIGOS; ++i) {
		    dibujarCubo(posicion_enemigo[i][0], 1.0f + rebote_enemigo, posicion_enemigo[i][1], 2.0f, 2.0f, 2.0f);
		}

        // --- DIBUJAR LAS BALAS ---
        glColor3f(1.0f, 1.0f, 0.0f);
        for (const auto& bala : balas) {
            dibujarCubo(bala.posicion[0], bala.posicion[1], bala.posicion[2], 0.2f, 0.2f, 0.2f);
        }

        // --- DIBUJAR EL ARMA CERCA DE LA CÁMARA ---
        glPushMatrix();
            glTranslatef(posicion_camara_x + direccion_camara_x * 0.5f, posicion_camara_y + altura_salto - 0.3f + direccion_camara_y * 0.5f, posicion_camara_z + direccion_camara_z * 0.5f);
            glRotatef(angulo_pitch, 1.0f, 0.0f, 0.0f);
            glRotatef(angulo_yaw + 90.0f, 0.0f, 1.0f, 0.0f);

            glEnable(GL_TEXTURE_2D);
            if (esta_animando_disparo && !pistola_animacion_texturas.empty()) {
                glBindTexture(GL_TEXTURE_2D, pistola_animacion_texturas[pistola_animacion_cuadro_actual % pistola_animacion_texturas.size()]);
            } else {
                glBindTexture(GL_TEXTURE_2D, texturaID_pistola);
            }
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.2f, -0.1f, 0.0f);
                glTexCoord2f(1.0f, 0.0f); glVertex3f(0.2f, -0.1f, 0.0f);
                glTexCoord2f(1.0f, 1.0f); glVertex3f(0.2f, 0.1f, 0.0f);
                glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.2f, 0.1f, 0.0f);
            glEnd();
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();

        // Restaurar matrices de la escena del juego
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        // Llamada al HUD en la escena del juego
        glDisable(GL_DEPTH_TEST);
        dibujarHUD();

        // Dibujar el minimapa
    	dibujarMinimapa(grosor_pared, altura_pared); // Pasa los valores aquí

        glEnable(GL_DEPTH_TEST);
    }

    glutSwapBuffers();
}

void actualizar(int value) {
    if (juego_terminado) return;

    rebote_enemigo += direccion_rebote * 0.01f;
    if (rebote_enemigo > 0.2f || rebote_enemigo < 0.0f)
        direccion_rebote *= -1;

    if (esta_saltando) {
        altura_salto += velocidad_salto;
        velocidad_salto -= GRAVEDAD;
        if (altura_salto <= 0.0f) {
            altura_salto = 0.0f;
            esta_saltando = false;
        }
    }

    if (verificarColision()) {
        vidas--;
        resetearPosicionJugador();
        if (vidas <= 0) {
            juego_terminado = true;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, actualizar, 0);
}

void manejarTeclas(unsigned char key, int x, int y) {
    if (!juego_iniciado) {
        if (key == ' ') {
            juego_iniciado = true;
            resetearPosicionJugador();
            vidas = 3;
            juego_terminado = false;
        }
        return; // No procesar otras teclas si el juego no ha comenzado
    }

    if (juego_terminado) return;
    float velocidad_movimiento = 1.03f;

    float derechaX = -direccion_camara_z;
    float derechaZ = direccion_camara_x;

    switch (key) {
        case 'w':
            posicion_camara_x += direccion_camara_x * velocidad_movimiento;
            posicion_camara_z += direccion_camara_z * velocidad_movimiento;
            break;
        case 's':
            posicion_camara_x -= direccion_camara_x * velocidad_movimiento;
            posicion_camara_z -= direccion_camara_z * velocidad_movimiento;
            break;
        case 'a':
            posicion_camara_x -= derechaX * velocidad_movimiento;
            posicion_camara_z -= derechaZ * velocidad_movimiento;
            break;
        case 'd':
            posicion_camara_x += derechaX * velocidad_movimiento;
            posicion_camara_z += derechaZ * velocidad_movimiento;
            break;
        case ' ': if (!esta_saltando) {
            esta_saltando = true;
            velocidad_salto = VELOCIDAD_SALTO_INICIAL;
        } break;
        case 'f': // Tecla para disparar
            float distancia_impacto;
            for (int i = 0; i < 3; ++i) {
                CajaColision caja_enemigo = obtenerCajaColisionEnemigo(i);
                if (rayoIntersectaCaja(posicion_camara_x, posicion_camara_y + altura_salto, posicion_camara_z,
                                     direccion_camara_x, direccion_camara_y, direccion_camara_z,
                                     caja_enemigo, distancia_impacto)) {
                    std::cout << "¡Impacto en el enemigo " << i << " a distancia " << distancia_impacto << "!" << std::endl;
                    break;
                }
            }
            break;
        case 27:
            exit(0);
    }
}

void inicializarRenderizado() {

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_FLAT);

}

void redimensionar(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
    ancho_pantalla = w;
    alto_pantalla = h;
    centro_X = ancho_pantalla / 2;
    centro_Y = alto_pantalla / 2;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(ancho_pantalla, alto_pantalla);
    glutCreateWindow("Prototipo de Juego FPS DOOM");

    glEnable(GL_DEPTH_TEST);

    inicializarRenderizado();
    texturaID_suelo = cargarTextura("neutral.tga"); // Cargar la textura del suelo
	texturaID_cara_doomguy = cargarTextura("rostro_doomguy.tga"); // Cargar la textura del doomguy


    glutDisplayFunc(dibujarEscena);
    glutKeyboardFunc(manejarTeclas);
    glutPassiveMotionFunc(movimientoMouse);
    glutTimerFunc(16, actualizar, 0);
    glutReshapeFunc(redimensionar);
    glutPassiveMotionFunc(movimientoMouse); // Registrar movimiento del mouse
    glutSetCursor(GLUT_CURSOR_NONE); // Ocultar el cursor

    glutMainLoop();

    return 0;
}    
