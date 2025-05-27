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
#include <string>



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

//============MENU
int modoVisual = 0;      // ELECCION DIA
int sonidoActivo = 0;    // ELECCION SONIDO

float rebote_enemigo = 0.2f;
float direccion_rebote = 1.0f;
const int NUM_ENEMIGOS = 8;
float posicion_enemigo[NUM_ENEMIGOS][2] = {
    {-5.0f, 5.0f},
    {5.0f, -5.0f},
    {0.0f, 7.0f},
    {-10.0f, -10.0f}, // Posición del enemigo 4
    {15.0f, 2.0f},      
	{25.0f, 2.0f}, 
	{55.0f, 2.0f} // Posición del enemigo 5
};
//============arma

bool esta_animando_disparo = false;

int ancho_pantalla = 1200;
int alto_pantalla = 800;

struct Posicion {
    float x;
    float z;
};

struct Bala {
    Posicion posicion;
};

// Declara una variable para la textura 
GLuint texturaID_pared; 
GLuint texturaID_techo; 
GLuint texturaID_suelo;
GLuint texturaID_cara_doomguy;
GLuint texturaID_pistola;
GLuint texturaHUD = 0;

std::vector<GLuint> pistola_animacion_texturas;
int pistola_animacion_cuadro_actual = 0;
int tiempo_inicio_animacion = 0;
int duracion_entre_cuadros = 80; // Milisegundos por cuadro

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


std::vector<GLuint> arma_frames;
int arma_frame_actual = 0;
float arma_tiempo = 0.0f;
float arma_duracion_frame = 0.1f;

void cargarFramesArma() {
    arma_frames.push_back(cargarTextura("pistola_0.png"));
    arma_frames.push_back(cargarTextura("pistola_1.png"));
    arma_frames.push_back(cargarTextura("pistola_2.png"));
    arma_frames.push_back(cargarTextura("pistola_3.png"));
}

void cargarFramesEscopeta() {
    arma_frames.push_back(cargarTextura("escopeta_0.png"));
    arma_frames.push_back(cargarTextura("escopeta_1.png"));
    arma_frames.push_back(cargarTextura("escopeta_2.png"));
    arma_frames.push_back(cargarTextura("escopeta_3.png"));
    arma_frames.push_back(cargarTextura("escopeta_4.png"));
    arma_frames.push_back(cargarTextura("escopeta_5.png"));
    arma_frames.push_back(cargarTextura("escopeta_6.png"));
    arma_frames.push_back(cargarTextura("escopeta_7.png"));
}

void cargarFramesRevolver() {
    arma_frames.push_back(cargarTextura("revolver_0.png"));
    arma_frames.push_back(cargarTextura("revolver_1.png"));
    arma_frames.push_back(cargarTextura("revolver_2.png"));
    arma_frames.push_back(cargarTextura("revolver_3.png"));
    arma_frames.push_back(cargarTextura("revolver_4.png"));
    arma_frames.push_back(cargarTextura("revolver_5.png"));
    arma_frames.push_back(cargarTextura("revolver_6.png"));
}


void cargarFramesCara() {
    arma_frames.push_back(cargarTextura("doomguy_0.png"));
    arma_frames.push_back(cargarTextura("doomguy_1.png"));
    arma_frames.push_back(cargarTextura("doomguy_2.png"));
    arma_frames.push_back(cargarTextura("doomguy_3.png"));
}



void dibujarArmaAnimada() {
    // --- Cambiar a proyección ortográfica 2D ---
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // --- Dibuja el arma centrada en la parte inferior de la pantalla ---
    float arma_ancho = 280; // Cambia esto según el tamaño real de tu imagen
    float arma_alto  = 280; // Cambia esto según el tamaño real de tu imagen
    float x = (ancho_pantalla - arma_ancho) / 2.0f;
    float y = 100; // parte inferior
            
	glDisable(GL_LIGHTING);        // Desactiva la iluminación global
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    if (!arma_frames.empty()) {
        glBindTexture(GL_TEXTURE_2D, arma_frames[arma_frame_actual]);
    }
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y);
	    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + arma_ancho, y);
	    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + arma_ancho, y + arma_alto);
	    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + arma_alto);
    glEnd();
    glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

    // --- Restaurar matrices ---
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void actualizarAnimacionArma(float deltaTime) {
    if (esta_animando_disparo) {
        arma_tiempo += deltaTime;
        if (arma_tiempo >= arma_duracion_frame) {
            arma_tiempo = 0.0f;
            arma_frame_actual++;
            if (arma_frame_actual >= arma_frames.size()) {
                arma_frame_actual = 0;        // Vuelve al frame 0 (reposo)
                esta_animando_disparo = false; // Termina la animación
            }
        }
    }
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



void manejarClickMouse(int button, int state, int x, int y) {
    // Solo actúa si el botón izquierdo es presionado, el juego está iniciado y no está terminado
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && juego_iniciado && !juego_terminado) {
        // --- Animación del disparo, igual que en 'f' ---
        esta_animando_disparo = true;
        arma_frame_actual = 0;
        arma_tiempo = 0.0f;

        // --- Chequea si impacta un enemigo ---
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
    }
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
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texturaHUD);
	glColor3f(1, 1, 1);  // Color blanco para no alterar el color de la textura
	
	glBegin(GL_QUADS);
	    glTexCoord2f(0, 0); glVertex2f(0, 0);                          // Inferior izquierda
	    glTexCoord2f(1, 0); glVertex2f(ancho_pantalla, 0);             // Inferior derecha
	    glTexCoord2f(1, 1); glVertex2f(ancho_pantalla, 100);           // Superior derecha
	    glTexCoord2f(0, 1); glVertex2f(0, 100);                        // Superior izquierda
	glEnd();

	glDisable(GL_TEXTURE_2D);

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
    static bool primer_vez = true;
    if (primer_vez) {
        glutWarpPointer(centro_X, centro_Y);
        primer_vez = false;
        return;
    }

    int dx = x - centro_X;
    int dy = centro_Y - y; // arriba es positivo

    if (dx == 0 && dy == 0)
        return;

    float sensibilidad = 0.1f; // Ajusta este valor según prefieras
    angulo_yaw   += dx * sensibilidad;
    angulo_pitch += dy * sensibilidad;

    // Limita el pitch para no dar la vuelta completa (para evitar "flip" vertical)
    if (angulo_pitch > 89.0f) angulo_pitch = 89.0f;
    if (angulo_pitch < -89.0f) angulo_pitch = -89.0f;

    // Actualiza la dirección de la cámara
    float radianes_yaw = angulo_yaw * M_PI / 180.0f;
    float radianes_pitch = angulo_pitch * M_PI / 180.0f;

    direccion_camara_x = cos(radianes_yaw) * cos(radianes_pitch);
    direccion_camara_y = sin(radianes_pitch);
    direccion_camara_z = sin(radianes_yaw) * cos(radianes_pitch);

    // Recentrar el mouse para que siempre esté en el centro (como FPS clásico)
    glutWarpPointer(centro_X, centro_Y);

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


void dibujar_pared_mapa(float offset_mapa_x, float offset_mapa_y, float escala_mapa, float offset_mundo, float x1, float z1, float x2, float z2) {
    glBegin(GL_LINES);
        glVertex2f(offset_mapa_x + (x1 + offset_mundo) * escala_mapa, offset_mapa_y + (z1 + offset_mundo) * escala_mapa);
        glVertex2f(offset_mapa_x + (x2 + offset_mundo) * escala_mapa, offset_mapa_y + (z2 + offset_mundo) * escala_mapa);
    glEnd();
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
    
    // Pared vertical izquierda
    for (float z = -40.0f; z <= 40.0f; z += 10.0f) {
        dibujar_pared_mapa(offset_mapa_x, offset_mapa_y, escala_mapa, offset_mundo, -40.0f, z, -40.0f, z + grosor_pared);
    }
	
	// Paredes horizontales desde la izquierda
    dibujar_pared_mapa(offset_mapa_x, offset_mapa_y, escala_mapa, offset_mundo, -40.0f, 30.0f, -40.0f + grosor_pared * 5, 30.0f);
    dibujar_pared_mapa(offset_mapa_x, offset_mapa_y, escala_mapa, offset_mundo, -40.0f, 10.0f, -40.0f + grosor_pared * 3, 10.0f);
    dibujar_pared_mapa(offset_mapa_x, offset_mapa_y, escala_mapa, offset_mundo, -40.0f, -10.0f, -40.0f + grosor_pared * 4, -10.0f);
    dibujar_pared_mapa(offset_mapa_x, offset_mapa_y, escala_mapa, offset_mundo, -40.0f, -30.0f, -40.0f + grosor_pared * 2, -30.0f);
    // Pared vertical de conexión
    for (float y = -20.0f; y <= 20.0f; y += 10.0f) {
        dibujar_pared_mapa(offset_mapa_x, offset_mapa_y, escala_mapa, offset_mundo, -20.0f, y, -20.0f, y + grosor_pared);
    }
	
    // Pared horizontal inferior derecha
    dibujar_pared_mapa(offset_mapa_x, offset_mapa_y, escala_mapa, offset_mundo, -10.0f, -40.0f, -10.0f + grosor_pared * 6, -40.0f);


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
bool mostrarMenu = false;
int opcionSeleccionada = -1;
//===========ILUMINACION


void configurarIluminacion() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat luz_dia[]     = { 1.0f, 1.0f, 1.0f, 1.0f };  // Blanca
    GLfloat luz_noche[]   = { 0.1f, 0.1f, 0.3f, 1.0f };  // Azul tenue

    GLfloat pos_luz[]     = { 0.0f, 10.0f, 10.0f, 1.0f };

    if (modoVisual == 0) {
        glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_dia);
        glLightfv(GL_LIGHT0, GL_SPECULAR, luz_dia);
    } else {
        glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_noche);
        glLightfv(GL_LIGHT0, GL_SPECULAR, luz_noche);
    }

    glLightfv(GL_LIGHT0, GL_POSITION, pos_luz);
}


void dibujarTexto(float x, float y, const std::string& texto) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < texto.length(); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, texto[i]);
    }
}

void onMenu(unsigned char tecla, int x, int y) {
switch (tecla) {
        case 'm': // Mostrar/Ocultar menú
        case 'M':
            mostrarMenu = !mostrarMenu;
            break;
        case '1':
            if (mostrarMenu) modoVisual = 0; // Día
            break;
        case '2':
            if (mostrarMenu) modoVisual = 1; // Noche
            break;
        case '3':
            if (mostrarMenu) sonidoActivo = 1;
            break;
        case '4':
            if (mostrarMenu) sonidoActivo = 0;
            break;
        case '5':
            if (mostrarMenu) exit(0);
            break;
    }
    glutPostRedisplay();
}

void crearMenu() {
    if (!mostrarMenu) return;

    // Guardar la matriz actual
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, ancho_ventana, 0, alto_ventana); // Coordenadas en píxeles

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST); // No profundidad para 2D
    glColor3f(0.0, 0.0, 0.0);

    // Fondo negro semitransparente del menú
    glBegin(GL_QUADS);
        glVertex2f(50, alto_ventana - 50);
        glVertex2f(350, alto_ventana - 50);
        glVertex2f(350, alto_ventana - 300);
        glVertex2f(50, alto_ventana - 300);
    glEnd();

    // Dibujar texto del menú
    glColor3f(1.0, 1.0, 1.0); // Blanco
    dibujarTexto(70, alto_ventana - 80, "MENU:");
    dibujarTexto(70, alto_ventana - 110, "1. MODO DIA");
    dibujarTexto(70, alto_ventana - 140, "2. MODO NOCHE");
    dibujarTexto(70, alto_ventana - 170, "3. ACTIVAR SONIDO");
    dibujarTexto(70, alto_ventana - 200, "4. DESACTIVAR SONIDO");
    dibujarTexto(70, alto_ventana - 230, "5. SALIR");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    // Restaurar las matrices originales
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void BordeDOOM(){
	glColor3f(0.0f,0.0f,0.0f);
	glLineWidth(10.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(-0.8f,0.72f);
		glVertex2f(-0.5f,0.71f);
		glVertex2f(-0.41f,0.63f);
		glVertex2f(-0.42f,0.25f);
		glVertex2f(-0.73f,-0.06f);
		glVertex2f(-0.79f,-0.06f);
		glVertex2f(-0.79f,-0.09f);
		glVertex2f(-0.77f,0.65f);
		glVertex2f(-0.8f,0.68f);
	glEnd();
	glBegin(GL_LINE_LOOP);
		glVertex2f(-0.41f,0.63f);
		glVertex2f(-0.33f,0.71f);
		glVertex2f(-0.14f,0.71f);
		glVertex2f(-0.05f,0.63f);
		glVertex2f(-0.06f,0.24f);
		glVertex2f(-0.22f,0.08f);
		glVertex2f(-0.25f,0.08f);
		glVertex2f(-0.42f,0.25f);
	glEnd();
	glBegin(GL_LINE_LOOP);
		glVertex2f(-0.05f,0.63f);
		glVertex2f(0.03f,0.71f);
		glVertex2f(0.22f,0.71f);
		glVertex2f(0.31f,0.63f);
		glVertex2f(0.3f,0.24f);
		glVertex2f(0.13f,0.08f);
		glVertex2f(0.1f,0.08f);
		glVertex2f(-0.06f,0.24f);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f(0.28f,0.71f);
		glVertex2f(0.51f,0.7f);
		glVertex2f(0.56f,0.65f);
		glVertex2f(0.61f,0.7f);
		glVertex2f(0.84f,0.7f);
		glVertex2f(0.84f,0.67f);
		glVertex2f(0.81f,0.64f);
		glVertex2f(0.81f,-0.1f);
		glVertex2f(0.67f,0.01f);
		glVertex2f(0.67f,0.4f);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f(0.67f,0.35f);
		glVertex2f(0.64f,0.35f);
		glVertex2f(0.64f,0.32f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.62f,0.32f);
		glVertex2f(0.62f,0.23f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.59f,0.23f);
		glVertex2f(0.59f,0.15f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.53f,0.15f);
		glVertex2f(0.53f,0.23f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.56f,0.09f);
		glVertex2f(0.56f,0.15f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.5f,0.23f);
		glVertex2f(0.5f,0.31f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.47f,0.32f);
		glVertex2f(0.47f,0.4f);
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f(0.45f,0.46f);
		glVertex2f(0.44f,0.1f);
		glVertex2f(0.3f,0.24f);
		glVertex2f(0.31f,0.65f);
		glVertex2f(0.28f,0.65f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.56f,0.65f);
		glVertex2f(0.56f,0.6f);
	glEnd();
	
	
}
void BordeDOO(){
	glColor3f(0.0f,0.0f,0.0f);
	glBegin(GL_QUADS);
		glVertex2f(-0.63f,0.27f);
		glVertex2f(-0.63f,0.54f);
		glVertex2f(-0.6f,0.54f);
		glVertex2f(-0.6f,0.27f);
	glEnd();
	glBegin(GL_QUADS);
		glVertex2f(-0.6f,0.54f);
		glVertex2f(-0.6f,0.29f);
		glVertex2f(-0.58f,0.29f);
		glVertex2f(-0.57f,0.54f);
	glEnd();
	
	glColor3f(0.792f,0.294f,0.149f);
	glLineWidth(10.0f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.64f,0.38f);
		glVertex2f(-0.64f,0.54f);//B
		glVertex2f(-0.56f,0.54f);
		glVertex2f(-0.56f,0.45f);
	glEnd();
	
	glColor3f(0.968f,0.792f,0.192f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.64f,0.38f);
		glVertex2f(-0.64f,0.26f);
		glVertex2f(-0.57f,0.29f);
		glVertex2f(-0.56f,0.45f);
	glEnd();
	
	glColor3f(0.0f,0.0f,1.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.65f,0.38f);
		glVertex2f(-0.74f,0.29f);
		glVertex2f(-0.73f,0.67f);
		glVertex2f(-0.65f,0.67f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.65f,0.67f);
		glVertex2f(-0.49f,0.67f);
		glVertex2f(-0.46f,0.64f);
		glVertex2f(-0.46f,0.55f);
		glVertex2f(-0.65f,0.55f);
	glEnd();
	glBegin(GL_TRIANGLES);
		glVertex2f(-0.46f,0.55f);
		glVertex2f(-0.55f,0.45f);
		glVertex2f(-0.55f,0.55f);
	glEnd();
}
void BordeD_RED(){
	glColor3f(1.0f,0.0f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.74f,0.18f);//G
		glVertex2f(-0.75f,-0.03f);//I
		glVertex2f(-0.47f,0.24f);//K
		glVertex2f(-0.47f,0.35f);//J
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.74f,0.29f);//A
		glVertex2f(-0.74f,0.18f);//G
		glVertex2f(-0.65f,0.23f);//n
		glVertex2f(-0.65f,0.38f);//n6
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.55f,0.45f);//H
		glVertex2f(-0.56f,0.29f);//n5
		glVertex2f(-0.47f,0.35f);//J
		glVertex2f(-0.46f,0.55f);//F
	glEnd();
}
void BordeO_RED(){
	glColor3f(1.0f,0.0f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.36f,0.55f);
		glVertex2f(-0.275f,0.47f);
		glVertex2f(-0.275f,0.15f);
		glVertex2f(-0.37f,0.25f);	
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.3f,0.29f);
		glVertex2f(-0.13f,0.21f);
		glVertex2f(-0.21f,0.12f);
		glVertex2f(-0.25f,0.12f);
		glVertex2f(-0.3f,0.17f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.18f,0.365f);
		glVertex2f(-0.1f,0.28f);
		glVertex2f(-0.1f,0.24f);
		glVertex2f(-0.13f,0.21f);
		glVertex2f(-0.19f,0.2f);
	glEnd();
}
void BordeOO_RED(){
	glColor3f(1.0f,0.0f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.01f,0.3f);
		glVertex2f(0.08f,0.38f);
		glVertex2f(0.08f,0.14f);
		glVertex2f(-0.01f,0.23f);	
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.06f,0.23f);
		glVertex2f(0.18f,0.28f);
		glVertex2f(0.25f,0.23f);
		glVertex2f(0.14f,0.12f);
		glVertex2f(0.11f,0.12f);
		glVertex2f(0.06f,0.16f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.18f,0.46f);
		glVertex2f(0.27f,0.55f);
		glVertex2f(0.26f,0.28f);
		glVertex2f(0.25f,0.23f);
		glVertex2f(0.17f,0.28f);
	glEnd();
}
void BordeM_RED(){
	glColor3f(1.0f,0.0f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(0.35f,0.56f);
		glVertex2f(0.4f,0.5f);
		glVertex2f(0.4f,0.18f);
		glVertex2f(0.35f,0.23f);	
	glEnd();
	glBegin(GL_TRIANGLES);
		glVertex2f(0.50f,0.47f);//C
		glVertex2f(0.62f,0.35f);
		glVertex2f(0.50f,0.35f);
		
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.52f,0.35f);
		glVertex2f(0.6f,0.35f);
		glVertex2f(0.6f,0.31f);
		glVertex2f(0.52f,0.31f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.54f,0.31f);
		glVertex2f(0.6f,0.31f);
		glVertex2f(0.6f,0.28f);
		glVertex2f(0.54f,0.28f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.71f,0.31f);
		glVertex2f(0.77f,0.26f);
		glVertex2f(0.77f,-0.01f);
		glVertex2f(0.71f,0.03f);
	glEnd();
	
}
void BordeM(){
	glColor3f(0.0f,0.0f,1.0f);
	glBegin(GL_POLYGON);
		glVertex2f(0.35f,0.665f);
		glVertex2f(0.52f,0.665f);
		glVertex2f(0.52f,0.56f);
		glVertex2f(0.35f,0.56f);	
	glEnd();
	glBegin(GL_TRIANGLES);
		glVertex2f(0.35f,0.56f);
		glVertex2f(0.4f,0.56f);
		glVertex2f(0.4f,0.5f);
	glEnd();
	glBegin(GL_TRIANGLES);
		glVertex2f(0.4f,0.56f);
		glVertex2f(0.55f,0.56f);
		glVertex2f(0.54f,0.43f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.54f,0.52f);
		glVertex2f(0.77f,0.52f);
		glVertex2f(0.62f,0.35f);
		glVertex2f(0.54f,0.43f);	
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.71f,0.49f);
		glVertex2f(0.77f,0.52f);
		glVertex2f(0.77f,0.26f);
		glVertex2f(0.71f,0.31f);	
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.57f,0.58f);
		glVertex2f(0.77f,0.58f);
		glVertex2f(0.77f,0.52f);
		glVertex2f(0.57f,0.51f);		
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.6f,0.66f);
		glVertex2f(0.77f,0.66f);
		glVertex2f(0.77f,0.58f);
		glVertex2f(0.6f,0.58f);		
	glEnd();
	
}
void letraD(){
	glColor3f(1.0f,0.3f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.76f,0.7f);
		glVertex2f(-0.79f,-0.05f);
		glVertex2f(-0.72f,-0.05f);
		glVertex2f(-0.7f,0.7f);
	glEnd();
	
	glBegin(GL_POLYGON);
		glVertex2f(-0.7f,0.7f);
		glVertex2f(-0.5f,0.7f);
		glVertex2f(-0.42f,0.64f);
		glVertex2f(-0.43f,0.24f);
		glVertex2f(-0.72f,-0.05f);
	glEnd();
	
	glColor3f(0.635f,0.635f,0.635f);
	glLineWidth(10.0f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.8f,0.69f);
		glVertex2f(-0.5f,0.68f);
		glVertex2f(-0.41f,0.61f);
	glEnd();
}
void letraO(){
	glColor3f(1.0f,0.3f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.42f,0.64f);
		glVertex2f(-0.33f,0.71f);
		glVertex2f(-0.13f,0.71f);
		glVertex2f(-0.06f,0.63f);
		glVertex2f(-0.06f,0.24f);
		glVertex2f(-0.23f,0.08f);
		glVertex2f(-0.26f,0.08f);
		glVertex2f(-0.43f,0.24f);
	glEnd();
	
	glColor3f(0.635f,0.635f,0.635f);
	glLineWidth(10.0f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.4f,0.61f);
		glVertex2f(-0.33f,0.68f);
		glVertex2f(-0.11f,0.675f);
	glEnd();
}
void letraO_2(){
	glColor3f(1.0f,0.3f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.06f,0.63f);
		glVertex2f(0.03f,0.71f);
		glVertex2f(0.22f,0.71f);
		glVertex2f(0.31f,0.63f);
		glVertex2f(0.3f,0.24f);
		glVertex2f(0.13f,0.08f);
		glVertex2f(0.1f,0.08f);
		glVertex2f(-0.06f,0.24f);
	glEnd();
	
	glColor3f(0.635f,0.635f,0.635f);
	glLineWidth(10.0f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.04f,0.62f);
		glVertex2f(0.03f,0.68f);
		glVertex2f(0.25f,0.68f);
	glEnd();
}
void letraM(){
	glColor3f(1.0f,0.3f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(0.31f,0.71f);
		glVertex2f(0.5f,0.7f);
		glVertex2f(0.56f,0.65f);
		glVertex2f(0.56f,0.46f);
		glVertex2f(0.31f,0.46f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.31f,0.46f);
		glVertex2f(0.45f,0.46f);
		glVertex2f(0.44f,0.1f);
		glVertex2f(0.3f,0.24f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.31f,0.46f);
		glVertex2f(0.45f,0.46f);
		glVertex2f(0.44f,0.1f);
		glVertex2f(0.3f,0.24f);
	glEnd();
	glBegin(GL_TRIANGLES);
		glVertex2f(0.44f,0.46f);
		glVertex2f(0.69f,0.46f);
		glVertex2f(0.56f,0.09f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.56f,0.65f);
		glVertex2f(0.61f,0.7f);
		glVertex2f(0.81f,0.7f);
		glVertex2f(0.81f,0.46f);
		glVertex2f(0.56f,0.46f);	
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.67f,0.46f);
		glVertex2f(0.81f,0.46f);
		glVertex2f(0.81f,-0.1f);
		glVertex2f(0.67f,0.01f);
	glEnd();
	
	glColor3f(0.635f,0.635f,0.635f);
	glLineWidth(10.0f);
	glBegin(GL_LINES);
		glVertex2f(0.28f,0.68f);
		glVertex2f(0.53f,0.68f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(0.59f,0.67f);
		glVertex2f(0.84f,0.67f);
	glEnd();
}
void MotosierraBorde(){
	glColor3f(1.0f,1.0f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.85f,-0.36f);
		glVertex2f(-0.22f,-0.36f);
		glVertex2f(-0.22f,-0.65f);
		glVertex2f(-0.71f,-0.67f);
		glVertex2f(-0.86f,-0.58f);
	glEnd();
	glColor3f(0.5f,0.5f,0.5f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.33f,-0.12f);
		glVertex2f(-0.19f,-0.04f);
		glVertex2f(-0.02f,-0.04f);
		glVertex2f(0.09f,-0.16f);
		glVertex2f(0.02f,-0.17f);
		glVertex2f(-0.27f,-0.17f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(0.02f,-0.17f);
		glVertex2f(0.09f,-0.16f);
		glVertex2f(0.09f,-0.37f);
		glVertex2f(0.02f,-0.37f);
	glEnd();
	glColor3f(0.0f,0.0f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.22f,-0.36f);
		glVertex2f(0.81f,-0.38f);
		glVertex2f(0.87f,-0.41f);
		glVertex2f(0.87f,-0.57f);
		glVertex2f(0.78f,-0.63f);
		glVertex2f(-0.22f,-0.62f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.43f,-0.65f);
		glVertex2f(-0.31f,-0.65f);
		glVertex2f(-0.3f,-0.7f);
		glVertex2f(-0.39f,-0.7f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.22f,-0.62f);
		glVertex2f(-0.08f,-0.62f);
		glVertex2f(-0.17f,-0.67f);
		glVertex2f(-0.23f,-0.65f);
	glEnd();
	
	glColor3f(0.5f,0.5f,0.5f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.85f,-0.36f);
		glVertex2f(-0.71f,-0.18f);
		glVertex2f(-0.5f,-0.12f);
		glVertex2f(-0.27f,-0.12f);
		glVertex2f(-0.09f,-0.26f);
		glVertex2f(-0.1f,-0.36f);
		glVertex2f(-0.85f,-0.36f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.41f,-0.71f);
		glVertex2f(-0.44f,-0.65f);
		glVertex2f(-0.44f,-0.62f);
		glVertex2f(-0.41f,-0.59f);
		glVertex2f(-0.36f,-0.62f);
		glVertex2f(-0.36f,-0.71f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.44f,-0.62f);
		glVertex2f(-0.5f,-0.59f);
		glVertex2f(-0.49f,-0.34f);
		glVertex2f(-0.41f,-0.34f);
		glVertex2f(-0.41f,-0.59f);
	glEnd();
	glColor3f(1.0f,0.0f,0.0f);
	glBegin(GL_POLYGON);
		glVertex2f(-0.84f,-0.38f);
		glVertex2f(-0.61f,-0.38f);
		glVertex2f(-0.59f,-0.52f);
		glVertex2f(-0.44f,-0.65f);
		glVertex2f(-0.5f,-0.66f);
		glVertex2f(-0.71f,-0.66f);
		glVertex2f(-0.85f,-0.58f);
		
	glEnd();
	glColor3f(0.0f,0.0f,0.0f);
	glLineWidth(10.0f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(-0.86f,-0.58f);
		glVertex2f(-0.85f,-0.36f);
		glVertex2f(-0.71f,-0.18f);
		glVertex2f(-0.5f,-0.12f);
		glVertex2f(-0.33f,-0.12f);
		glVertex2f(-0.19f,-0.04f);
		glVertex2f(-0.02f,-0.04f);
		glVertex2f(0.09f,-0.16f);
		glVertex2f(0.08f,-0.35f);
		glVertex2f(0.8f,-0.36f);
		glVertex2f(0.88f,-0.42f);
		glVertex2f(0.88f,-0.54f);
		glVertex2f(0.77f,-0.65f);
		glVertex2f(-0.08f,-0.64f);
		glVertex2f(-0.17f,-0.67f);
		glVertex2f(-0.31f,-0.66f);
		glVertex2f(-0.3f,-0.7f);
		glVertex2f(-0.39f,-0.7f);
		glVertex2f(-0.43f,-0.66f);
		glVertex2f(-0.71f,-0.67f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.68f,-0.33f);
		glVertex2f(-0.49f,-0.33f);
		glVertex2f(-0.54f,-0.27f);
		glVertex2f(-0.54f,-0.2f);
		glVertex2f(-0.63f,-0.2f);
		glVertex2f(-0.68f,-0.25f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(-0.22f,-0.36f);
		glVertex2f(-0.22f,-0.62f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(-0.36f,-0.36f);
		glVertex2f(-0.36f,-0.47f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(-0.39f,-0.42f);
		glVertex2f(-0.39f,-0.56f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(-0.34f,-0.69f);
		glVertex2f(-0.34f,-0.62f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(-0.37f,-0.62f);
		glVertex2f(-0.37f,-0.58f);
	glEnd();
	glEnd();
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.1f,-0.36f);
		glVertex2f(-0.1f,-0.26f);
		glVertex2f(-0.25f,-0.17f);
		glVertex2f(0.02f,-0.18f);
		glVertex2f(0.02f,-0.365f);
	glEnd();
	glColor3f(0.0f,1.0f,0.0f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(-0.18f,-0.38f);
		glVertex2f(0.81f,-0.39f);
		glVertex2f(0.85f,-0.42f);
		glVertex2f(0.85f,-0.55f);
		glVertex2f(0.78f,-0.62f);
		glVertex2f(-0.21f,-0.61f);
	glEnd();
	glColor3f(0.0f,0.0f,0.0f);
	glBegin(GL_LINE_STRIP);
		glVertex2f(0.04f,-0.13f);
		glVertex2f(-0.27f,-0.12f);
		glVertex2f(-0.27f,-0.17f);
	glEnd();
	glBegin(GL_LINES);
		glVertex2f(-0.34f,-0.21f);
		glVertex2f(-0.29f,-0.21f);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex2f(-0.37f,-0.22f);
		glVertex2f(-0.29f,-0.22f);
		glVertex2f(-0.29f,-0.36f);
		glVertex2f(-0.37f,-0.36f);
	glEnd();
}

void dibujarEscena() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    configurarIluminacion();

    if (!juego_iniciado) {
        // --- PANTALLA DE INICIO ---
        glDisable(GL_DEPTH_TEST);

        // Fondo degradado (coordenadas en píxeles)
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Fondo degradado
        glBegin(GL_QUADS);
            glColor3f(0.2f, 0.1f, 0.0f); glVertex2f(0, 0);
            glColor3f(0.4f, 0.2f, 0.0f); glVertex2f(ancho_pantalla, 0);
            glColor3f(0.2f, 0.1f, 0.0f); glVertex2f(ancho_pantalla, alto_pantalla);
            glColor3f(0.4f, 0.2f, 0.0f); glVertex2f(0, alto_pantalla);
        glEnd();
        // Texto instrucción
        glColor3f(0.8f, 1.6f, 0.4f);
		const char* texto_instruccion = "PULSA ESPACIO PARA JUGAR";
		int ancho_texto_instr = 0;
		for (const char* c = texto_instruccion; *c; ++c)
		    ancho_texto_instr += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c); // Fuente grande
		float pos_x_instr = ancho_pantalla / 2 - ancho_texto_instr / 2;
		float pos_y_instr = 30;  // ? Casi en el borde inferior
		glRasterPos2f(pos_x_instr, pos_y_instr);
		for (const char* c = texto_instruccion; *c; ++c)
		    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
		    
        glPopMatrix(); 
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(-1.0, 1.0, -1.0, 1.0); 

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        letraD();
        letraO();
        letraO_2();
        letraM();
        BordeDOO();

        glPushMatrix();
            glTranslatef(-0.29f, 0.0f, 0.0f);
            glRotatef(-4.0f, 0.0f, 0.0f, 1.0f);
            glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
            glTranslatef(0.57f, 0.0f, 0.0f);
            BordeDOO();
        glPopMatrix();

        glPushMatrix();
            glTranslatef(0.73f, 0.0f, 0.0f);
            BordeDOO();
        glPopMatrix();

        BordeD_RED();
        BordeM();
        BordeDOOM();         // ? ahora sí se verá
        BordeO_RED();
        BordeOO_RED();
        BordeM_RED();
        MotosierraBorde();

        glPopMatrix(); // restaurar modelo
        glMatrixMode(GL_PROJECTION);
        glPopMatrix(); // restaurar proyección
        glMatrixMode(GL_MODELVIEW);
    }else {
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
		
		// --- DIBUJAR EL ARMA CERCA DE LA CÁMARA ---
      	dibujarArmaAnimada();

        // Restaurar matrices de la escena del juego
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        // Llamada al HUD en la escena del juego
        glDisable(GL_LIGHTING);        // Desactiva la iluminación global
        glDisable(GL_DEPTH_TEST);
        dibujarHUD();
		crearMenu();

        // Dibujar el minimapa
    	dibujarMinimapa(grosor_pared, altura_pared); // Pasa los valores aquí

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_LIGHT0);          
    }

    glutSwapBuffers();
}

// Variable global para el último tiempo de actualización
float ultimoTiempo = 0.0f;

void actualizar(int value) {
    // Calcula deltaTime
    float tiempoActual = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // segundos
    float deltaTime = tiempoActual - ultimoTiempo;
    ultimoTiempo = tiempoActual;

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

    // --- ¡Actualiza la animación del arma aquí! ---
    actualizarAnimacionArma(deltaTime);

    glutPostRedisplay();
    glutTimerFunc(20, actualizar, 0);
}


void manejarTeclas(unsigned char key, int x, int y) 
	{
	    if (key == 'm' || key == 'M') {
	        mostrarMenu = !mostrarMenu;
	        glutPostRedisplay();
	        return;
	    }
	
	    if (mostrarMenu) 
		{
	        switch (key) {
	            case '1':
	                modoVisual = 0; // Día
	                break;
	            case '2':
	                modoVisual = 1; // Noche
	                break;
	            case '3':
	                sonidoActivo = 1;
	                break;
	            case '4':
	                sonidoActivo = 0;
	                break;
	            case '5':
	                exit(0);
	                break;
	        }
	        glutPostRedisplay();
	        return; 
	    }
	    
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
	    float velocidad_movimiento = 0.8f;
	
	    float derechaX = -direccion_camara_z;
	    float derechaZ = direccion_camara_x;
	
			// Calcula el YAW en radianes
		float radianes_yaw = angulo_yaw * M_PI / 180.0f;
		float frente_x = cos(radianes_yaw);
		float frente_z = sin(radianes_yaw);
	
	    switch (key) {
	        case 'w':
		        posicion_camara_x += frente_x * velocidad_movimiento;
		        posicion_camara_z += frente_z * velocidad_movimiento;
		        break;
		    case 's':
		        posicion_camara_x -= frente_x * velocidad_movimiento;
		        posicion_camara_z -= frente_z * velocidad_movimiento;
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
			    esta_animando_disparo = true;
			    arma_frame_actual = 0; // reinicia animación
			    arma_tiempo = 0.0f;
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



void display(void) {
    if (modoVisual == 0) {  // Día
        glColor3f(0.0, 0.0, 0.0); 
    } else {  // Noche
        glColor3f(1.0, 1.0, 1.0); 
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (modoVisual == 0)
        dibujarTexto(-0.5f, 0.3f, "Modo: DIA");
    else
        dibujarTexto(-0.5f, 0.3f, "Modo: NOCHE");

    if (sonidoActivo)
        dibujarTexto(-0.5f, 0.1f, "Sonido: ACTIVADO");
    else
        dibujarTexto(-0.5f, 0.1f, "Sonido: DESACTIVADO");

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(ancho_pantalla, alto_pantalla);
    glutCreateWindow("Prototipo de Juego FPS DOOM");
    glutMouseFunc(manejarClickMouse); 

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL); // Para que el color del material se mezcle con la luz
    glShadeModel(GL_SMOOTH);

    inicializarRenderizado();

    texturaID_suelo = cargarTextura("neutral.tga");
    texturaHUD = cargarTextura("hud.tga");
    texturaID_cara_doomguy = cargarTextura("rostro_doomguy.tga");
    
   // Calcula deltaTime como ya lo haces
    cargarFramesArma();

   
    glutPostRedisplay();
    glutDisplayFunc(dibujarEscena);

    glutKeyboardFunc(manejarTeclas);
    glutPassiveMotionFunc(movimientoMouse);
    glutTimerFunc(20, actualizar, 0);
    glutReshapeFunc(redimensionar);
    
    glutSetCursor(GLUT_CURSOR_NONE);
    crearMenu();

    glutMainLoop();
    return 0;
}
