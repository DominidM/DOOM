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
#include <mmsystem.h>
#define MAX_ENEMIGOS 20 // O la cantidad máxima que desees
#pragma comment(lib, "winmm.lib")
#else
#include <sys/time.h>
#endif
#define M_PI 3.14159265358979323846
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <string>


// ==== CONSTANTES ===
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
float posicion_camara_x = 0.0f, posicion_camara_y = 1.0f, posicion_camara_z = 0.0f;
float altura_salto = 0.10f, velocidad_salto = 0.10f;
bool esta_saltando = false;
int municion = 50; 
int vidas = 3;
bool juego_terminado = false;
bool juego_iniciado = false; // Nuevo estado para la pantalla de inicio

//============MENU 1
int modoVisual = 0;      // ELECCION DIA
int sonidoActivo = 0;    // ELECCION SONIDO
bool mostrarMenu = false;
int opcionSeleccionada = -1;

//============MENU 2
typedef enum {
    MODO_DIA,
    MODO_NOCHE,
    AUDIO_ON,
    AUDIO_OFF,
    SALIR,
} opcionesMenu;


//============arma
bool esta_animando_disparo = false;
int ancho_pantalla = 1200;
int alto_pantalla = 800;

// Declara una variable para la textura 
GLuint texturaID_pared; 
GLuint texturaID_techo; 
GLuint texturaID_suelo;
GLuint texturaID_cielo;
GLuint texturaHUD = 0;

struct PosicionJugador {
    float x;
    float z;
};
PosicionJugador jugador = {0.0f, 0.0f}; // Inicializa donde quieras que empiece el jugador

// Estados posibles para el enemigo
enum EstadoEnemigo { CAMINAR, ATACAR, MORIR, MUERTO };

struct Enemigo {
    float x, y, z;
    int vida;
    int estado;
    int frameActual;
    float tiempoAnimacion;
    bool activo;
};

// Array de enemigos y contador de enemigos activos:
Enemigo enemigos[MAX_ENEMIGOS];
const int numEnemigos = 3;


enum TipoArma { PISTOLA = 0, ESCOPETA = 1, REVOLVER = 2 };
int arma_actual = PISTOLA;


GLuint cargarTextura(const char* nombreArchivo) {
    int width, height, channels;
    unsigned char* data = stbi_load(nombreArchivo, &width, &height, &channels, STBI_rgb_alpha);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Parámetros de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}


//VECTORERS PARA ARMAS
std::vector<GLuint> frames_pistola;
std::vector<GLuint> frames_escopeta;
std::vector<GLuint> frames_revolver;

//Vectores para los frames de animación
std::vector<GLuint> frames_enemigo_walk;
std::vector<GLuint> frames_enemigo_attack;
std::vector<GLuint> frames_enemigo_die;
std::vector<GLuint> frames_cara;


int cara_frame_actual = 0;
float cara_tiempo = 0.0f;
float cara_duracion_frame = 0.12f; // Velocidad de animación, ajusta a gusto

// Para el render y la animación del arma actual:
std::vector<GLuint>* arma_frames_actual = &frames_pistola;
int arma_frame_actual = 0;
float arma_tiempo = 0.0f;
float arma_duracion_frame = 0.1f;


// AJUSTES DE SONIDO
void reproducirMusica(const char* archivo) {
    char comando[40];
    sprintf(comando, "open \"%s\" type mpegvideo alias miMusica", archivo);
    mciSendString(comando, NULL, 0, NULL);
    mciSendString("play miMusica repeat", NULL, 0, NULL); // repeat para bucle
}
void detenerMusica() {
    mciSendString("stop miMusica", NULL, 0, NULL);
    mciSendString("close miMusica", NULL, 0, NULL);
}


// FRAMES 
void cargarFramesEnemigo() {
    // Animación de caminar
    frames_enemigo_walk.push_back(cargarTextura("mod1_0.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_0.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_1.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_1.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_0.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_0.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_1.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_1.png"));
    frames_enemigo_walk.push_back(cargarTextura("mod1_0.png"));
    
    // Animación de atacar
    frames_enemigo_attack.push_back(cargarTextura("mod1_3.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_3.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_4.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_4.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_5.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_6.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_6.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_6.png"));
    frames_enemigo_attack.push_back(cargarTextura("mod1_5.png"));


    // Animación de morir
    frames_enemigo_die.push_back(cargarTextura("mod1_7.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_8.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_9.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_10.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_11.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_12.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_13.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_14.png"));
    frames_enemigo_die.push_back(cargarTextura("mod1_14.png"));
}
void cargarFramesPistola() {
    frames_pistola.push_back(cargarTextura("pistola_0.png"));
    frames_pistola.push_back(cargarTextura("pistola_1.png"));
    frames_pistola.push_back(cargarTextura("pistola_1.png"));
    frames_pistola.push_back(cargarTextura("pistola_2.png"));
    frames_pistola.push_back(cargarTextura("pistola_3.png"));
}
void cargarFramesEscopeta() {
    frames_escopeta.push_back(cargarTextura("escopeta_0.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_1.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_2.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_3.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_4.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_5.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_6.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_7.png"));
    frames_escopeta.push_back(cargarTextura("escopeta_7.png"));
}
void cargarFramesRevolver() {
    frames_revolver.push_back(cargarTextura("revolver_0.png"));
    frames_revolver.push_back(cargarTextura("revolver_1.png"));
    frames_revolver.push_back(cargarTextura("revolver_2.png"));
    frames_revolver.push_back(cargarTextura("revolver_3.png"));
    frames_revolver.push_back(cargarTextura("revolver_4.png"));
    frames_revolver.push_back(cargarTextura("revolver_5.png"));
    frames_revolver.push_back(cargarTextura("revolver_6.png"));
}
void cargarFramesCara() {
    frames_cara.push_back(cargarTextura("doomguy_0.png"));  // 0 abierto
    frames_cara.push_back(cargarTextura("doomguy_1.png"));  // 1 cerrado
    frames_cara.push_back(cargarTextura("doomguy_1.png"));  // 2 izquierda
    frames_cara.push_back(cargarTextura("doomguy_0.png"));  // 3 derecha
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_1.png"));
    frames_cara.push_back(cargarTextura("doomguy_1.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_2.png"));  
    frames_cara.push_back(cargarTextura("doomguy_2.png"));
    frames_cara.push_back(cargarTextura("doomguy_2.png"));
    frames_cara.push_back(cargarTextura("doomguy_2.png"));
    frames_cara.push_back(cargarTextura("doomguy_2.png"));
    frames_cara.push_back(cargarTextura("doomguy_2.png"));
    frames_cara.push_back(cargarTextura("doomguy_2.png"));
    frames_cara.push_back(cargarTextura("doomguy_2.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_1.png"));
    frames_cara.push_back(cargarTextura("doomguy_1.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_0.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
    frames_cara.push_back(cargarTextura("doomguy_3.png"));
}


// ACTUALIZAR ANIMACIONES 

GLuint obtenerTexturaEnemigo(const Enemigo& enemigo) {
    switch (enemigo.estado) {
        case CAMINAR: return frames_enemigo_walk[enemigo.frameActual];
        case ATACAR:  return frames_enemigo_attack[enemigo.frameActual];
        case MORIR:   return frames_enemigo_die[enemigo.frameActual];
        default:      return 0;
    }
}
void actualizarAnimacionCara(float deltaTime) {
    cara_tiempo += deltaTime;
    if (cara_tiempo >= cara_duracion_frame) {
        cara_tiempo = 0.0f;
        cara_frame_actual++;
        if (cara_frame_actual >= frames_cara.size()) {
            cara_frame_actual = 0; // Vuelve al primer frame
        }
    }
}
void actualizarEstadoEnemigo(Enemigo& enemigo, float jugador_x, float jugador_z) {
    // Si ya está muerto, no hace nada
    if (!enemigo.activo || enemigo.estado == MUERTO) return;

    // Si la vida es 0 o menos, cambia a MORIR si no lo está ya
    if (enemigo.vida <= 0 && enemigo.estado != MORIR && enemigo.estado != MUERTO) {
        enemigo.estado = MORIR;
        enemigo.tiempoAnimacion = 0.0f;
        return;
    }

    // Calcula distancia al jugador
    float dx = jugador_x - enemigo.x;
    float dz = jugador_z - enemigo.z;
    float distancia = sqrt(dx * dx + dz * dz);

    if (enemigo.estado == MORIR) return; // Está muriendo, no hace nada más

    // Cambia de estado según la distancia
    if (distancia < 5.0f) {
        enemigo.estado = ATACAR;
    } else {
        enemigo.estado = CAMINAR;
    }
}
void actualizarAnimacionEnemigo(Enemigo& enemigo, float deltaTime) {
    if (!enemigo.activo || enemigo.estado == MUERTO) return;
    enemigo.tiempoAnimacion += deltaTime;
    int totalFrames = 1;
    float frameDuration = 0.1f;

    switch (enemigo.estado) {
        case CAMINAR:
            totalFrames = frames_enemigo_walk.size();
            enemigo.frameActual = int(enemigo.tiempoAnimacion / frameDuration) % totalFrames;
            break;
        case ATACAR:
            totalFrames = frames_enemigo_attack.size();
            enemigo.frameActual = int(enemigo.tiempoAnimacion / frameDuration) % totalFrames;
            break;
        case MORIR:
            totalFrames = frames_enemigo_die.size();
            enemigo.frameActual = int(enemigo.tiempoAnimacion / frameDuration);
            if (enemigo.frameActual >= totalFrames) {
                enemigo.frameActual = totalFrames - 1;
                enemigo.estado = MUERTO;
                enemigo.activo = false;
            }
            break;
        default:
            enemigo.frameActual = 0;
    }
}
void dibujarTextoSombreado(float x, float y, const char* texto, void* fuente, float r, float g, float b) {
    // Sombra
    glColor3f(0,0,0); 
    glRasterPos2f(x+2, y-2);
    for(const char* c=texto; *c; c++) glutBitmapCharacter(fuente, *c);
    // Texto principal
    glColor3f(r,g,b);
    glRasterPos2f(x, y);
    for(const char* c=texto; *c; c++) glutBitmapCharacter(fuente, *c);
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

    // ¡OJO! Usa el puntero correcto y verifica que no esté vacío
    if (arma_frames_actual && !arma_frames_actual->empty()) {
        GLuint textura = (*arma_frames_actual)[arma_frame_actual];
        glBindTexture(GL_TEXTURE_2D, textura);

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(x + arma_ancho, y);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(x + arma_ancho, y + arma_alto);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + arma_alto);
        glEnd();
    }
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
            if (arma_frames_actual && arma_frame_actual >= arma_frames_actual->size()) {
                arma_frame_actual = 0;        // Vuelve al frame 0 (reposo)
                esta_animando_disparo = false; // Termina la animación
            }
        }
    }
}
void resetearPosicionJugador() {
    // Actualiza struct jugador
    jugador.x = 8.0f;
    jugador.z = 0.0f;

    // Actualiza también la cámara si es necesario
    posicion_camara_x = jugador.x;
    posicion_camara_y = 0.80f; // Altura de la cámara (jugador de pie)
    posicion_camara_z = jugador.z;

    // Opcional: resetear ángulos de cámara
    angulo_yaw = 0.0f;
    angulo_pitch = 0.0f;

    // Opcional: resetear otras variables de movimiento o estado
    esta_saltando = false;
    altura_salto = 0.0f;
}
void reproducirSonidoArma(const char* archivo) {
    // Cierra el sonido anterior, si lo hay
    mciSendString("close sonidoArma", NULL, 0, NULL);
    // Abre el nuevo archivo como alias "sonidoArma"
    char comando[256];
    sprintf(comando, "open \"%s\" type mpegvideo alias sonidoArma", archivo);
    mciSendString(comando, NULL, 0, NULL);
    // Reproduce solo una vez (sin repeat)
    mciSendString("play sonidoArma", NULL, 0, NULL);
}
void manejarClickMouse(int button, int state, int x, int y) {
    // Permite iniciar el juego con click si aún no ha iniciado
    if (!juego_iniciado && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        juego_iniciado = true;
        resetearPosicionJugador();
        vidas = 3;
        juego_terminado = false;
        return;
    }

    // Solo actúa si el botón izquierdo es presionado, el juego está iniciado y no está terminado
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && juego_iniciado && !juego_terminado) {
        // --- Animación del disparo, igual que en 'f' ---
        esta_animando_disparo = true;
        arma_frame_actual = 0;
        arma_tiempo = 0.0f;

        // --- Reproduce el sonido correspondiente al arma actual ---
        if (arma_actual == PISTOLA) {
            reproducirSonidoArma("pistola.mp3");
        } else if (arma_actual == ESCOPETA) {
            reproducirSonidoArma("escopeta.mp3");
        } else if (arma_actual == REVOLVER) {
            reproducirSonidoArma("revolver.mp3");
        }

    }
}
void dibujarHUD(int municion) {

    // Cambiar a modo 2D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // --- Barra inferior estilo DOOM ---
    // Fondo metálico oscuro
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glColor3f(0.12f, 0.12f, 0.13f); // gris oscuro
        glVertex2f(0, 0);
        glVertex2f(ancho_pantalla, 0);
        glColor3f(0.18f, 0.18f, 0.22f); // gris menos oscuro arriba
        glVertex2f(ancho_pantalla, 100);
        glVertex2f(0, 100);
    glEnd();

    // Borde superior brillante
    glBegin(GL_QUADS);
        glColor3f(0.7f, 0.7f, 0.82f);
        glVertex2f(0, 97);
        glVertex2f(ancho_pantalla, 97);
        glColor3f(0.3f, 0.3f, 0.42f);
        glVertex2f(ancho_pantalla, 100);
        glVertex2f(0, 100);
    glEnd();

    // --- Textura decorativa del HUD (si tienes textura) ---
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaHUD);
    glColor3f(1,1,1);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(ancho_pantalla, 0);
        glTexCoord2f(1, 1); glVertex2f(ancho_pantalla, 100);
        glTexCoord2f(0, 1); glVertex2f(0, 100);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // --- Separadores metálicos laterales ---
    glBegin(GL_QUADS);
        glColor3f(0.5f, 0.5f, 0.6f);
        glVertex2f(128, 0); glVertex2f(134, 0);
        glVertex2f(134, 100); glVertex2f(128, 100);
        glVertex2f(ancho_pantalla-128, 0); glVertex2f(ancho_pantalla-134, 0);
        glVertex2f(ancho_pantalla-134, 100); glVertex2f(ancho_pantalla-128, 100);
    glEnd();

    // --- Recuadro para la cara del Doomguy ---
    float ancho_cara = 120;
    float alto_cara = 90;
    float posicion_cara_x = ancho_pantalla / 2 - ancho_cara / 2;
    float posicion_cara_y = 5;

    // Borde recuadro (gris oscuro/borde metálico)
    glColor3f(0.4f, 0.4f, 0.48f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(posicion_cara_x-4, posicion_cara_y-4);
        glVertex2f(posicion_cara_x+ancho_cara+4, posicion_cara_y-4);
        glVertex2f(posicion_cara_x+ancho_cara+4, posicion_cara_y+alto_cara+4);
        glVertex2f(posicion_cara_x-4, posicion_cara_y+alto_cara+4);
    glEnd();

    // --- Cara animada (Doomguy) ---
    glEnable(GL_TEXTURE_2D);
    glColor3f(1,1,1);
    glBindTexture(GL_TEXTURE_2D, frames_cara[cara_frame_actual]);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(posicion_cara_x, posicion_cara_y);                    // Inferior izquierda
        glTexCoord2f(1, 1); glVertex2f(posicion_cara_x + ancho_cara, posicion_cara_y);       // Inferior derecha
        glTexCoord2f(1, 0); glVertex2f(posicion_cara_x + ancho_cara, posicion_cara_y + alto_cara); // Superior derecha
        glTexCoord2f(0, 0); glVertex2f(posicion_cara_x, posicion_cara_y + alto_cara);        // Superior izquierda
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // --- Texto de vidas con sombra y color rojo DOOM ---
    char buffer[50];
    sprintf(buffer, "VIDAS");
    dibujarTextoSombreado(40, 80, buffer, GLUT_BITMAP_HELVETICA_18, 0.9,0.2,0.2);
    sprintf(buffer, "%d", vidas);
    dibujarTextoSombreado(60, 48, buffer, GLUT_BITMAP_TIMES_ROMAN_24, 1,1,1);

    // --- Texto de munición con sombra y color amarillo ---
    sprintf(buffer, "MUNICION");
    dibujarTextoSombreado(ancho_pantalla-110, 80, buffer, GLUT_BITMAP_HELVETICA_18, 1,0.9,0.2);
	sprintf(buffer, "%d", municion); // Usa tu variable real    dibujarTextoSombreado(ancho_pantalla-140, 48, buffer, GLUT_BITMAP_TIMES_ROMAN_24, 1,1,1);
    dibujarTextoSombreado(ancho_pantalla-75, 48, buffer, GLUT_BITMAP_TIMES_ROMAN_24, 1,1,1);
    const char* nombre_arma = "PISTOLA";
    switch(arma_actual) {
        case 0: nombre_arma = "PISTOLA"; break;
        case 1: nombre_arma = "ESCOPETA"; break;
        case 2: nombre_arma = "REVOLVER"; break;
        default: break;
    }
    dibujarTextoSombreado(ancho_pantalla/2-54, 95, nombre_arma, GLUT_BITMAP_HELVETICA_18, 0.8,0.8,0.85);

    // --- Restaurar matrices ---
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



    // Restaurar matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


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
void onMenu_2(int opcion) {
    switch (opcion) {
        case MODO_DIA:
            modoVisual = 0;
            break;
        case MODO_NOCHE:
            modoVisual = 1;
            break;
        case AUDIO_ON:
   			reproducirMusica("soundtrack.mp3");
            break;
        case AUDIO_OFF:
            detenerMusica();
            break;
        case SALIR:
            exit(0);
            break;
        default:
            break;
    }
    glutPostRedisplay();
}
void crearMenu_2(void) {
    int menuOpciones, menuAudio, menuPrincipal;

    // Submenús
    menuOpciones = glutCreateMenu(onMenu_2);
    glutAddMenuEntry("DIA", MODO_DIA);
    glutAddMenuEntry("NOCHE", MODO_NOCHE);
    
    menuAudio = glutCreateMenu(onMenu_2);
    glutAddMenuEntry("ACTIVAR SONIDO", AUDIO_ON);
    glutAddMenuEntry("DESACTIVAR SONIDO", AUDIO_OFF);

    // Menú principal
    menuPrincipal = glutCreateMenu(onMenu_2);
    glutAddSubMenu("Opciones", menuOpciones);
    glutAddSubMenu("Audio", menuAudio);
    glutAddMenuEntry("Salir", SALIR);

    // Asociar menú al botón derecho del mouse
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// DIBUJO 2D 
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


void moverEnemigoHaciaJugador(Enemigo& enemigo, float jugador_x, float jugador_z, float velocidad, float deltaTime) {
    float dx = jugador_x - enemigo.x;
    float dz = jugador_z - enemigo.z;
    float distancia = sqrt(dx * dx + dz * dz);
    if (distancia > 0.05f) { // evita división por cero
        enemigo.x += (dx / distancia) * velocidad * deltaTime;
        enemigo.z += (dz / distancia) * velocidad * deltaTime;
    }
}
void inicializarEnemigos() {
    enemigos[0].x = -10; enemigos[0].y = 0; enemigos[0].z = 10;
    enemigos[0].vida = 100; enemigos[0].estado = CAMINAR; enemigos[0].activo = true;
    enemigos[1].x = 0; enemigos[1].y = 0; enemigos[1].z = 20;
    enemigos[1].vida = 100; enemigos[1].estado = CAMINAR; enemigos[1].activo = true;
    enemigos[2].x = 15; enemigos[2].y = 0; enemigos[2].z = -5;
    enemigos[2].vida = 100; enemigos[2].estado = CAMINAR; enemigos[2].activo = true;
    // ... inicializa los demás campos según tu struct Enemigo
}
void dibujarEnemigoBillboard(const Enemigo& enemigo, float jugador_x, float jugador_z) {
    if (!enemigo.activo) return;
    float dx = jugador_x - enemigo.x;
    float dz = jugador_z - enemigo.z;
    float angle = atan2(dx, dz) * 180.0f / M_PI;
    GLuint textura = obtenerTexturaEnemigo(enemigo);

    glPushMatrix();
        glTranslatef(enemigo.x, enemigo.y, enemigo.z);
        glRotatef(angle, 0, 1, 0);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D, textura);
		glColor4f(1, 1, 1, 1); // Importante: alpha=1
		
		float escala = 2.50f; // Cambia este valor para ajustar el tamaño

		glBegin(GL_QUADS);
		    glTexCoord2f(0, 1); glVertex3f(-1 * escala, 0, 0);
		    glTexCoord2f(1, 1); glVertex3f(1 * escala, 0, 0);
		    glTexCoord2f(1, 0); glVertex3f(1 * escala, 2 * escala, 0);
		    glTexCoord2f(0, 0); glVertex3f(-1 * escala, 2 * escala, 0);
		glEnd();
				
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}










// Variable global para el último tiempo de actualización
float ultimoTiempo = 0.0f;


// Variables de estado de teclas para movimiento fluido
bool tecla_w = false, tecla_a = false, tecla_s = false, tecla_d = false;

// Llama a este en glutKeyboardFunc
void manejarTeclas(unsigned char key, int x, int y) 
{
    // Mostrar/Ocultar menú con 'M' o 'm'
    if (key == 'm' || key == 'M') {
        mostrarMenu = !mostrarMenu;
        glutPostRedisplay();
        return;
    }

    // --- Menú 2D activo: control de opciones ---
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
                reproducirMusica("soundtrack.mp3");
                break;
            case '4':
                detenerMusica();
                break;
            case '5':
            case 27: // ESC
                exit(0);
                break;
        }
        glutPostRedisplay();
        return; 
    }
    
    // --- Iniciar juego con ENTER o ESPACIO ---
    if (!juego_iniciado) {
        if (key == ' ' || key == 13) { // 13 es ENTER
            juego_iniciado = true;
            resetearPosicionJugador();
            vidas = 3;
            juego_terminado = false;
        }
        return; // No procesar otras teclas si el juego no ha comenzado
    }

    // --- Si el juego terminó, ignora teclas ---
    if (juego_terminado) return;
    
    // --- Cambio de arma instantáneo ---
    if (key == '1') {
        arma_actual = PISTOLA;
        arma_frames_actual = &frames_pistola;
        arma_frame_actual = 0;
        return;
    }
    if (key == '2') {
        arma_actual = ESCOPETA;
        arma_frames_actual = &frames_escopeta;
        arma_frame_actual = 0;
        return;
    }
    if (key == '3') {
        arma_actual = REVOLVER;
        arma_frames_actual = &frames_revolver;
        arma_frame_actual = 0;
        return;
    }

    // --- Movimiento fluido: activa flags de teclas ---
    switch (key) {
        case 'w': tecla_w = true; break;
        case 'a': tecla_a = true; break;
        case 's': tecla_s = true; break;
        case 'd': tecla_d = true; break;
        case ' ':
            if (!esta_saltando) {
                esta_saltando = true;
                velocidad_salto = VELOCIDAD_SALTO_INICIAL;
            }
            break;
        case 'f': // Disparo
            esta_animando_disparo = true;
            arma_frame_actual = 0; // reinicia animación
            arma_tiempo = 0.0f;
            {
               
            }
            break;
        case 27: // ESC
            exit(0);
    }
}

// Para movimiento fluido: llama a esto en glutKeyboardUpFunc
void manejarTeclasUp(unsigned char key, int x, int y)
{
    switch (key) {
        case 'w': tecla_w = false; break;
        case 'a': tecla_a = false; break;
        case 's': tecla_s = false; break;
        case 'd': tecla_d = false; break;
    }
}

// Llama a esto en tu ciclo de actualización (timer o idle)
void actualizarMovimientoJugador(float deltaTime)
{
    float velocidad_movimiento = 10.0f * deltaTime; 
    float radianes_yaw = angulo_yaw * M_PI / 180.0f;
    float frente_x = cos(radianes_yaw);
    float frente_z = sin(radianes_yaw);

    // Vector lateral normalizado
    float derechaX = -frente_z;
    float derechaZ =  frente_x;
    float magnitud = sqrt(derechaX * derechaX + derechaZ * derechaZ);
    if (magnitud != 0.0f) {
        derechaX /= magnitud;
        derechaZ /= magnitud;
    }

    bool movimiento = false;

    if (tecla_w) {
        posicion_camara_x += frente_x * velocidad_movimiento;
        posicion_camara_z += frente_z * velocidad_movimiento;
        movimiento = true;
    }
    if (tecla_s) {
        posicion_camara_x -= frente_x * velocidad_movimiento;
        posicion_camara_z -= frente_z * velocidad_movimiento;
        movimiento = true;
    }
    if (tecla_a) {
        posicion_camara_x -= derechaX * velocidad_movimiento;
        posicion_camara_z -= derechaZ * velocidad_movimiento;
        movimiento = true;
    }
    if (tecla_d) {
        posicion_camara_x += derechaX * velocidad_movimiento;
        posicion_camara_z += derechaZ * velocidad_movimiento;
        movimiento = true;
    }

    // Sincroniza lógica de jugador
    if (movimiento) {
        jugador.x = posicion_camara_x;
        jugador.z = posicion_camara_z;
    }
}

void actualizar(int value) {
    // Calcula deltaTime seguro
    float tiempoActual = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // segundos
    float deltaTime = tiempoActual - ultimoTiempo;
    ultimoTiempo = tiempoActual;

    if (juego_terminado) {
        glutPostRedisplay();
        glutTimerFunc(20, actualizar, 0); // Sigue actualizando para HUD o animaciones
        return;
    }



    // Lógica de salto del jugador
    if (esta_saltando) {
        altura_salto += velocidad_salto;
        velocidad_salto -= GRAVEDAD;
        if (altura_salto <= 0.0f) {
            altura_salto = 0.0f;
            esta_saltando = false;
        }
    }



    // Animación de arma y DOOMGUY (cara)
    actualizarAnimacionArma(deltaTime);
    actualizarAnimacionCara(deltaTime);
    actualizarMovimientoJugador(deltaTime);

    // --- Actualización de enemigos ---
    float velocidad = 2.0f; // o el valor que prefieras

	for (int i = 0; i < numEnemigos; ++i) {
	    actualizarEstadoEnemigo(enemigos[i], jugador.x, jugador.z);
	    if (enemigos[i].estado == CAMINAR) {
	        moverEnemigoHaciaJugador(enemigos[i], jugador.x, jugador.z, velocidad, deltaTime);
	    }
	    actualizarAnimacionEnemigo(enemigos[i], deltaTime);
	    printf("Jugador en (%.2f, %.2f)\n", jugador.x, jugador.z);
	}
	
	
    glutPostRedisplay();
    glutTimerFunc(20, actualizar, 0);
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
		const char* texto_instruccion = "DALE CLIC PARA JUGAR";
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
        glBindTexture(GL_TEXTURE_2D, texturaID_techo); // Enlaza la textura

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-50.0f, 5.0f, -50.0f);
            glTexCoord2f(10.0f, 0.0f); glVertex3f(50.0f, 5.0f, -50.0f);
            glTexCoord2f(10.0f, 10.0f); glVertex3f(50.0f, 5.0f, 50.0f);
            glTexCoord2f(0.0f, 10.0f); glVertex3f(-50.0f, 5.0f, 50.0f);
        glEnd();

        glDisable(GL_TEXTURE_2D);		     
		
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
		
		// --- FAROLILLOS EN LAS ESQUINAS ---
		glDisable(GL_TEXTURE_2D);
		float farol_altura = 5.2f;
		float farol_radio = 0.4f;
		float farol_color[4][3] = {
		    {1.0f, 0.9f, 0.5f}, // Amarillo cálido
		    {0.7f, 0.9f, 1.0f}, // Azul
		    {1.0f, 0.7f, 0.7f}, // Rojo
		    {0.7f, 1.0f, 0.7f}  // Verde claro
		};
		float esquina[4][2] = {
		    {-50.0f, -50.0f},
		    {50.0f, -50.0f},
		    {-50.0f, 50.0f},
		    {50.0f, 50.0f}
		};
		for (int i = 0; i < 4; ++i) {
		    glPushMatrix();
		    glTranslatef(esquina[i][0], farol_altura, esquina[i][1]);
		    glColor3fv(farol_color[i]);
		    glutSolidSphere(farol_radio, 16, 16);
		    glColor3f(0.2f, 0.18f, 0.05f);
		    glTranslatef(0.0f, -farol_altura, 0.0f);
			GLUquadric* quad = gluNewQuadric();
			gluCylinder(quad, 0.13, 0.13, farol_altura, 16, 2);
			gluDeleteQuadric(quad);

		    glPopMatrix();
		}
		
		// --- PAREDES INTERIORES (color variado y efecto desgaste) ---
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texturaID_pared);
		// Puedes alternar colores para cada bloque o con ruido
		srand(42);
		for (int pared = 0; pared < 20; pared++) {
		    float r = 0.88f + 0.10f * (rand() % 2); // blanco o gris muy suave
		    float g = 0.88f + 0.10f * (rand() % 2);
		    float b = 0.86f + 0.12f * (rand() % 3);
		    glColor3f(r, g, b);
		
		    // Ejemplo: usa tus mismas coordenadas de paredes interiores aquí,
		    // o pon aleatoriamente algunas internas llamativas:
		    float x = (rand() % 70) - 35.0f;
		    float z = (rand() % 70) - 35.0f;
		    glBegin(GL_QUADS);
		        glTexCoord2f(0.0f, 1.0f); glVertex3f(x, 0.0f, z);
		        glTexCoord2f(1.0f, 1.0f); glVertex3f(x + grosor_pared, 0.0f, z);
		        glTexCoord2f(1.0f, 0.0f); glVertex3f(x + grosor_pared, altura_pared, z);
		        glTexCoord2f(0.0f, 0.0f); glVertex3f(x, altura_pared, z);
		    glEnd();
		}
		
		// --- SOMBRA BÁSICA BAJO LAS PAREDES (oscurece el suelo junto a la base de las paredes) ---
		glDisable(GL_TEXTURE_2D);
		for (float i = -50.0f; i <= 50.0f; i += grosor_pared) {
		    glColor4f(0.1f, 0.1f, 0.1f, 0.4f); // Sombra semitransparente
		    glBegin(GL_QUADS);
		        glVertex3f(i, 0.01f, -50.0f);
		        glVertex3f(i + grosor_pared, 0.01f, -50.0f);
		        glVertex3f(i + grosor_pared, 0.01f, -50.7f);
		        glVertex3f(i, 0.01f, -50.7f);
		    glEnd();
		}
		
		// --- FIN MAPA LLAMATIVO ---

       	
		for (int i = 0; i < numEnemigos; ++i) {
		    printf("Dibujando enemigo %d en (%.2f, %.2f, %.2f)\n", i, enemigos[i].x, enemigos[i].y, enemigos[i].z);
		    dibujarEnemigoBillboard(enemigos[i], jugador.x, jugador.z);
		}
		
		
		// --- DIBUJAR EL ARMA CERCA DE LA CÁMARA ---
      	dibujarArmaAnimada();

        // Restaurar matrices de la escena del juego
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glMatrixMode(GL_MODELVIEW);

        // Llamada al HUD en la escena del juego
        glDisable(GL_LIGHTING);        // Desactiva la iluminación global
        glDisable(GL_DEPTH_TEST);
        dibujarHUD(municion);
		crearMenu();

        // Dibujar el minimapa
        
        
    	dibujarMinimapa(grosor_pared, altura_pared); // Pasa los valores aquí

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_LIGHT0);          
    }

    glutSwapBuffers();
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
	texturaID_cielo = cargarTextura("cielo.tga");
    texturaID_techo = cargarTextura("techoInfierno.tga"); // Carga la textura

   // Calcula deltaTime como ya lo haces
	cargarFramesPistola();
	cargarFramesEscopeta();
	cargarFramesRevolver();
   	cargarFramesCara();
	cargarFramesEnemigo();
   
    glutPostRedisplay();
    glutDisplayFunc(dibujarEscena);

    glutPassiveMotionFunc(movimientoMouse);
    glutKeyboardFunc(manejarTeclas);
	glutKeyboardUpFunc(manejarTeclasUp);

    glutTimerFunc(20, actualizar, 0);
    glutReshapeFunc(redimensionar);
    
    glutSetCursor(GLUT_CURSOR_NONE);
    inicializarEnemigos();  // SOLO aquí. No la llames nunca dentro de dibujar o actualizar.
    crearMenu();
	crearMenu_2();
	
    glutMainLoop();
    return 0;
}
