#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ==== CONSTANTES ====
const float VELOCIDAD_MOVIMIENTO = 0.14f;
const float VELOCIDAD_SALTO_INICIAL = 0.07f;
const float GRAVEDAD = 0.01f;
const float RADIO_JUGADOR = 0.6f;
const float DISTANCIA_COLISION_CUADRADA = RADIO_JUGADOR * RADIO_JUGADOR;

// ==== C�MARA ====
float angulo_yaw = -90.0f; // inicial mirando hacia -Z
float angulo_pitch = 0.0f; // sin inclinaci�n vertical
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
float posicion_enemigo[3][2] = {
    {-5.0f, 5.0f},
    {5.0f, -5.0f},
    {0.0f, 7.0f},
};

int ancho_pantalla = 1200;
int alto_pantalla = 800;

struct Bala {
    float posicion[3];
    float direccion[3];
};

GLuint texturaID_suelo;
GLuint texturaID_cara_doomguy;
GLuint texturaID_pistola;
std::vector<GLuint> pistola_animacion_texturas;
int pistola_animacion_cuadro_actual = 0;
bool esta_animando_disparo = false;
int tiempo_inicio_animacion = 0;
int duracion_entre_cuadros = 50; // Milisegundos por cuadro
std::vector<Bala> balas;

void cargarAnimacionPistola(const char* archivo_gif) {
    // --- L�gica simulada para cargar la animaci�n ---
    printf("Cargando animaci�n de pistola desde %s\n", archivo_gif);
    // *** �IMPORTANTE! ***
    // Aqu� DEBES implementar la l�gica real para cargar los cuadros del GIF.
    // Esto requerir� una biblioteca externa para decodificar GIFs (como libgif).
    // El siguiente es un ejemplo SIMULADO para que el resto del c�digo funcione
    // sin errores, pero NO CARGA un GIF real.
    for (int i = 0; i < 5; ++i) {
        GLuint textura_frame = 0; // Simula la carga de un cuadro
        pistola_animacion_texturas.push_back(textura_frame);
    }
    // --- Reemplaza la simulaci�n con tu implementaci�n real de carga de GIF ---
}

//Funci�n para cargar la textura
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

// Estructura para representar la caja de colisi�n de un objeto (enemigo)
struct CajaColision {
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

// Funci�n para obtener la caja de colisi�n de un enemigo (ajusta seg�n tu representaci�n)
CajaColision obtenerCajaColisionEnemigo(int indice_enemigo) {
    CajaColision caja;
    float tamano = 1.0f; // Asumiendo que tus cubos enemigos tienen lado 2.0, el "radio" es 1.0
    caja.minX = posicion_enemigo[indice_enemigo][0] - tamano;
    caja.maxX = posicion_enemigo[indice_enemigo][0] + tamano;
    caja.minY = 0.0f;     // Ajusta si tus enemigos no est�n a nivel del suelo
    caja.maxY = 2.0f;     // Ajusta seg�n la altura de tus enemigos
    caja.minZ = posicion_enemigo[indice_enemigo][1] - tamano;
    caja.maxZ = posicion_enemigo[indice_enemigo][1] + tamano;
    return caja;
}

// Funci�n para verificar si un rayo intersecta con una caja de colisi�n
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

    t = tMin; // Distancia al punto de intersecci�n
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

       // Texto de vidas (m�s grande)
    char buffer[50];
    sprintf(buffer, "Vidas: %d", vidas);
    glColor3f(1.0f, 0.0f, 0.0f); // Rojo
    glRasterPos2f(30, 75); // Ajust� la posici�n vertical
    for (char* c = buffer; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c); // Fuente m�s grande (alternativa)
    }

    // --- CENTRO INFERIOR DEL HUD (Cara de Doomguy - Ajustada) ---
    glColor3f(1.0f, 1.0f, 1.0f); // Establecer color blanco para la textura
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaID_cara_doomguy);

    float ancho_cara = 120;
    float alto_cara = 90;
    float posicion_cara_x = ancho_pantalla / 2 - ancho_cara / 2;
    float posicion_cara_y = 5; // Ajust� la posici�n vertical

    float sprite_ancho_atlas = 1.0f / 8.0f; // Ancho de cada sprite en el atlas
    float sprite_alto_atlas = 0.8f / 5.35f;  // Alto de cada sprite en el atlas
    float zoom_factor_u = 0.910f; // Factor de "zoom" horizontal (mayor valor = m�s zoom)
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

    // Texto de munici�n (m�s grande y ajustado)
    sprintf(buffer, "Municion: %d", 42); // puedes hacerlo variable
    glColor3f(1.0f, 1.0f, 0.0f); // Amarillo
    glRasterPos2f(ancho_pantalla - 180, 75); // Mover m�s a la derecha y ajustar vertical
    for (char* c = buffer; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c); // Fuente m�s grande (alternativa)
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

void dibujarMinimapa() {
    // Cambiar a modo 2D
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, ancho_pantalla, 0, alto_pantalla);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Definir el tama�o y la posici�n del minimapa
    float tamano_mapa = 200.0f;
    float margen_derecho = 30.0f;
    float margen_superior = 30.0f;
    float posicion_mapa_x = ancho_pantalla - tamano_mapa - margen_derecho;
    float posicion_mapa_y = alto_pantalla - tamano_mapa - margen_superior;

    // Dibujar el fondo del minimapa
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
        glVertex2f(posicion_mapa_x, posicion_mapa_y);
        glVertex2f(posicion_mapa_x + tamano_mapa, posicion_mapa_y);
        glVertex2f(posicion_mapa_x + tamano_mapa, posicion_mapa_y + tamano_mapa);
        glVertex2f(posicion_mapa_x, posicion_mapa_y + tamano_mapa);
    glEnd();

    // Escalar las coordenadas del mundo al espacio del minimapa
    float escala_mapa = tamano_mapa / 100.0f; // Asumiendo que tu mundo tiene ~100x100 unidades

    // Dibujar al jugador como un punto amarillo
    glColor3f(1.0f, 1.0f, 0.0f);
    float jugador_mapa_x = posicion_mapa_x + (posicion_camara_x + 50.0f) * escala_mapa;
    float jugador_mapa_y = posicion_mapa_y + (posicion_camara_z + 50.0f) * escala_mapa;
    float tamano_jugador = 5.0f;
    glBegin(GL_QUADS);
        glVertex2f(jugador_mapa_x - tamano_jugador / 2, jugador_mapa_y - tamano_jugador / 2);
        glVertex2f(jugador_mapa_x + tamano_jugador / 2, jugador_mapa_y - tamano_jugador / 2);
        glVertex2f(jugador_mapa_x + tamano_jugador / 2, jugador_mapa_y + tamano_jugador / 2);
        glVertex2f(jugador_mapa_x - tamano_jugador / 2, jugador_mapa_y + tamano_jugador / 2);
    glEnd();

    // Dibujar a los enemigos como puntos rojos
    glColor3f(1.0f, 0.0f, 0.0f);
    float tamano_enemigo = 5.0f;
    for (int i = 0; i < 3; ++i) {
        float enemigo_mapa_x = posicion_mapa_x + (posicion_enemigo[i][0] + 50.0f) * escala_mapa;
        float enemigo_mapa_y = posicion_mapa_y + (posicion_enemigo[i][1] + 50.0f) * escala_mapa;
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
            glColor3f(0.2f, 0.1f, 0.0f); glVertex2f(0, 0);      // Marr�n oscuro
            glColor3f(0.4f, 0.2f, 0.0f); glVertex2f(ancho_pantalla, 0); // Marr�n m�s claro
            glColor3f(0.2f, 0.1f, 0.0f); glVertex2f(ancho_pantalla, alto_pantalla); // Marr�n oscuro
            glColor3f(0.4f, 0.2f, 0.0f); glVertex2f(0, alto_pantalla);  // Marr�n m�s claro
        glEnd();

        // Dibujar el t�tulo "AVENTURA �PICA" (ejemplo)
        glColor3f(1.0f, 0.8f, 0.0f); // Amarillo dorado
        const char* texto_titulo = "DOOM - GRUPO 11 - T1"; // Cambia el t�tulo aqu�
        int ancho_texto_titulo = 0;
        for (const char* c = texto_titulo; *c; ++c) {
            ancho_texto_titulo += glutBitmapWidth(GLUT_BITMAP_9_BY_15, *c); // Fuente retro
        }
        float posicion_texto_x = ancho_pantalla / 2 - ancho_texto_titulo / 2;
        float posicion_texto_y = alto_pantalla / 2 + 30; // Un poco m�s arriba
        glRasterPos2f(posicion_texto_x, posicion_texto_y);
        for (const char* c = texto_titulo; *c; ++c) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
        }

        // Dibujar la instrucci�n "PULSA ESPACIO PARA JUGAR" (ejemplo)
        glColor3f(0.8f, 0.6f, 0.0f); // Amarillo m�s apagado
        const char* texto_instruccion = "PULSA ESPACIO PARA JUGAR"; // Cambia la instrucci�n
        int ancho_texto_instruccion = 0;
        for (const char* c = texto_instruccion; *c; ++c) {
            ancho_texto_instruccion += glutBitmapWidth(GLUT_BITMAP_8_BY_13, *c); // Fuente m�s peque�a
        }
        float posicion_instruccion_x = ancho_pantalla / 2 - ancho_texto_instruccion / 2;
        float posicion_instruccion_y = alto_pantalla / 2 - 30; // Un poco m�s abajo
        glRasterPos2f(posicion_instruccion_x, posicion_instruccion_y);
        for (const char* c = texto_instruccion; *c; ++c) {
            glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
        }

        // Restaurar matrices
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);


    } else {
        glEnable(GL_DEPTH_TEST); // Volver a habilitar la prueba de profundidad para la escena 3D
        // Dibujar la escena del juego normal en perspectiva 3D
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
		gluPerspective(45.0, (double)ancho_pantalla / (double)alto_pantalla, 1.0, 200.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        gluLookAt(
            posicion_camara_x, posicion_camara_y + altura_salto, posicion_camara_z,
            posicion_camara_x + direccion_camara_x, posicion_camara_y + altura_salto + direccion_camara_y, posicion_camara_z + direccion_camara_z,
            0.0f, 1.0f, 0.0f
        );

        // Suelo con la textura completa
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



        // Techo
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_QUADS);
            glVertex3f(-50.0f, 5.0f, -50.0f);
            glVertex3f(50.0f, 5.0f, -50.0f);
            glVertex3f(50.0f, 5.0f, 50.0f);
            glVertex3f(-50.0f, 5.0f, 50.0f);
        glEnd();

        // Paredes
        glColor3f(0.5f, 0.5f, 0.5f);
        dibujarLineaPared(-50.0f, true);
        dibujarLineaPared(50.0f, true);
        dibujarLineaPared(-50.0f, false);
        dibujarLineaPared(50.0f, false);

        glColor3f(1.0f, 0.0f, 0.0f);
        for (int i = 0; i < 3; ++i) {
            glPushMatrix();
            glTranslatef(posicion_enemigo[i][0], 1.0f + rebote_enemigo, posicion_enemigo[i][1]);
            glutSolidCube(2.0f);
            glPopMatrix();
        }

        if (juego_terminado) {
            dibujarTexto(ancho_pantalla / 2 - 100, alto_pantalla / 2, "Juego Terminado"); // Centr� el texto
        }


		 // Dibujar el arma cerca de la c�mara
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
        dibujarMinimapa();

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
    float velocidad_movimiento = 0.14f;

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
                    std::cout << "�Impacto en el enemigo " << i << " a distancia " << distancia_impacto << "!" << std::endl;
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
    glutCreateWindow("Prototipo de Juego FPS");

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
