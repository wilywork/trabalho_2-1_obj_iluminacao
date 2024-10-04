#include <iostream>
#include <fstream>
#include <vector>
#include <GL/freeglut.h>
#include <string>
#include <sstream>

using namespace std;

// variaveis globais
unsigned int modeloObjeto;
vector<vector<float>> vertices, normais, coordenadasTextura;
vector<vector<int>> faces;
float rotacaoObjeto;

const double PI = 3.14159265358979323846;
float deslocamentoX = 0.0f;
float deslocamentoY = 0.0f;
float escala = 1.0f;
float angulo = 0.0f;

// variaveis para a textura
GLuint texturaID;

// estrutura para armazenar dados de BMP (do professor)
struct BitMapFile {
    int sizeX;
    int sizeY;
    unsigned char* data;
};

// funcao para carregar imagem BMP (do professor)
BitMapFile* getBMPData(string filename) {
    BitMapFile* bmp = new BitMapFile;
    unsigned int size, offset, headerSize;

    // ler o arquivo de entrada
    ifstream infile(filename.c_str(), ios::binary);

    // verificar se o arquivo foi aberto corretamente
    if (!infile) {
        cout << "falha ao abrir o arquivo de textura: " << filename << endl;
        exit(1);
    }

    // pegar o ponto inicial de leitura
    infile.seekg(10);
    infile.read((char*)&offset, 4);

    // pegar o tamanho do cabecalho do bmp
    infile.read((char*)&headerSize, 4);

    // pegar a altura e largura da imagem no cabecalho do bmp
    infile.seekg(18);
    infile.read((char*)&bmp->sizeX, 4);
    infile.read((char*)&bmp->sizeY, 4);

    // alocar o buffer para a imagem
    size = bmp->sizeX * bmp->sizeY * 24;
    bmp->data = new unsigned char[size];

    // ler a informacao da imagem
    infile.seekg(offset);
    infile.read((char*)bmp->data, size);

    // reverte a cor de bgr para rgb
    int temp;
    for (unsigned int i = 0; i < size; i += 3) {
        temp = bmp->data[i];
        bmp->data[i] = bmp->data[i + 2];
        bmp->data[i + 2] = temp;
    }

    return bmp;
}

// funcao para carregar a textura externa (do professor)
void carregarTextura(const char* nomeArquivo) {
    BitMapFile* image = getBMPData(nomeArquivo);

    glGenTextures(1, &texturaID);
    glBindTexture(GL_TEXTURE_2D, texturaID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->sizeX, image->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data);

    // liberar memoria
    delete[] image->data;
    delete image;
}

// funcao para carregar o arquivo obj
void carregarObjeto(string nomeArquivo) {
    ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open()) {
        cout << "arquivo nao encontrado: " << nomeArquivo << endl;
        exit(1);
    }

    string tipo;
    while (arquivo >> tipo) {
        if (tipo == "v") {
            // leitura de vertices
            vector<float> vertice(3);
            arquivo >> vertice[0] >> vertice[1] >> vertice[2];
            vertices.push_back(vertice);
        }
        else if (tipo == "vn") {
            // leitura de normais
            vector<float> normal(3);
            arquivo >> normal[0] >> normal[1] >> normal[2];
            normais.push_back(normal);
        }
        else if (tipo == "vt") {
            // leitura de coordenadas de textura
            vector<float> coordenadaTextura(2);
            arquivo >> coordenadaTextura[0] >> coordenadaTextura[1];
            // inverter a coordenada T
            coordenadaTextura[1] = 1.0f - coordenadaTextura[1];
            coordenadasTextura.push_back(coordenadaTextura);
        }
        else if (tipo == "f") {
            // leitura de faces (armazenar indices de vertices, texturas e normais)
            string vertice_str;
            vector<int> faceVertices; // armazena temporariamente todos os indices de uma face

            // ler o restante da linha apos 'f'
            getline(arquivo, vertice_str);
            if (vertice_str.empty()) {
                // caso a face esteja na mesma linha que o tipo, ler novamente
                getline(arquivo, vertice_str);
            }

            // processar cada vertice da face
            size_t pos = 0;
            size_t len = vertice_str.length();
            while (pos < len) {
                // ignorar espacos
                while (pos < len && vertice_str[pos] == ' ') pos++;

                if (pos >= len) break;

                // encontrar o proximo espaco ou fim da string
                size_t start = pos;
                while (pos < len && vertice_str[pos] != ' ') pos++;
                size_t end = pos;

                // extrair o token v/vt/vn
                string token = vertice_str.substr(start, end - start);

                // inicializar indices
                int indiceVertice = -1, indiceTextura = -1, indiceNormal = -1;

                // encontrar as posicoes dos '/'
                size_t pos1 = token.find('/');
                if (pos1 == string::npos) {
                    // somente indice de vertice
                    indiceVertice = stoi(token);
                }
                else {
                    size_t pos2 = token.find('/', pos1 + 1);
                    if (pos2 == string::npos) {
                        // formato v/vt
                        string verticeIdxStr = token.substr(0, pos1);
                        string texturaIdxStr = token.substr(pos1 + 1);

                        if (!verticeIdxStr.empty()) {
                            indiceVertice = stoi(verticeIdxStr);
                        }
                        if (!texturaIdxStr.empty()) {
                            indiceTextura = stoi(texturaIdxStr);
                        }
                    }
                    else {
                        // formato v/vt/vn ou v//vn
                        string verticeIdxStr = token.substr(0, pos1);
                        string texturaIdxStr = "";
                        if (pos2 > pos1 + 1) {
                            texturaIdxStr = token.substr(pos1 + 1, pos2 - pos1 - 1);
                        }
                        string normalIdxStr = token.substr(pos2 + 1);

                        if (!verticeIdxStr.empty()) {
                            indiceVertice = stoi(verticeIdxStr);
                        }
                        if (!texturaIdxStr.empty()) {
                            indiceTextura = stoi(texturaIdxStr);
                        }
                        if (!normalIdxStr.empty()) {
                            indiceNormal = stoi(normalIdxStr);
                        }
                    }
                }

                // ajustar indices negativos
                if (indiceVertice < 0) {
                    indiceVertice = vertices.size() + indiceVertice;
                }
                else {
                    indiceVertice -= 1;
                }

                if (indiceTextura < 0) {
                    indiceTextura = coordenadasTextura.size() + indiceTextura;
                }
                else if (indiceTextura > 0) {
                    indiceTextura -= 1;
                }

                if (indiceNormal < 0) {
                    indiceNormal = normais.size() + indiceNormal;
                }
                else if (indiceNormal > 0) {
                    indiceNormal -= 1;
                }

                // armazenar os indices
                faceVertices.push_back(indiceVertice);
                faceVertices.push_back(indiceTextura);
                faceVertices.push_back(indiceNormal);
            }

            // triangulacao da face caso tenha mais de 3 vertices
            size_t numVertices = faceVertices.size() / 3;
            if (numVertices < 3) {
                cout << "face com numero de vertices invalido ou nao suportado: " << numVertices << " vertices." << endl;
                continue;
            }

            for (size_t i = 1; i < numVertices - 1; ++i) {
                vector<int> triangulo(9);

                // primeiro vertice do triangulo
                triangulo[0] = faceVertices[0];
                triangulo[1] = faceVertices[1];
                triangulo[2] = faceVertices[2];

                // segundo vertice do triangulo
                triangulo[3] = faceVertices[i * 3];
                triangulo[4] = faceVertices[i * 3 + 1];
                triangulo[5] = faceVertices[i * 3 + 2];

                // terceiro vertice do triangulo
                triangulo[6] = faceVertices[(i + 1) * 3];
                triangulo[7] = faceVertices[(i + 1) * 3 + 1];
                triangulo[8] = faceVertices[(i + 1) * 3 + 2];

                // adicionar o triangulo a lista de faces
                faces.push_back(triangulo);
            }
        }
        else {
            // ignorar outros tipos de linhas
            string restoLinha;
            getline(arquivo, restoLinha);
            continue;
        }
    }
    arquivo.close();

    // centralizar o objeto
    if (!vertices.empty()) {
        // inicializar min e max com o primeiro vertice
        float minX = vertices[0][0], maxX = vertices[0][0];
        float minY = vertices[0][1], maxY = vertices[0][1];
        float minZ = vertices[0][2], maxZ = vertices[0][2];

        // encontrar os limites
        for (const auto& vertice : vertices) {
            if (vertice[0] < minX) minX = vertice[0];
            if (vertice[0] > maxX) maxX = vertice[0];
            if (vertice[1] < minY) minY = vertice[1];
            if (vertice[1] > maxY) maxY = vertice[1];
            if (vertice[2] < minZ) minZ = vertice[2];
            if (vertice[2] > maxZ) maxZ = vertice[2];
        }

        // calcular o centro da bounding box
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;

        // centralizar os vertices
        for (auto& vertice : vertices) {
            vertice[0] -= centerX;
            vertice[1] -= centerY;
            vertice[2] -= centerZ;
        }

        cout << "objeto centralizado na origem." << endl;
    }
    else {
        cout << "nenhum vertice encontrado para centralizar." << endl;
    }

    // compilar a lista de display para desenhar o objeto
    modeloObjeto = glGenLists(1);
    glNewList(modeloObjeto, GL_COMPILE);
    {
        glBegin(GL_TRIANGLES);  // renderizar as faces como triangulos
        for (const auto& face : faces) {
            for (int i = 0; i < 3; i++) {
                int indiceVertice = face[i * 3];
                int indiceTextura = face[i * 3 + 1];
                int indiceNormal = face[i * 3 + 2];

                // aplicar as coordenadas de textura se disponiveis
                if (indiceTextura >= 0 && indiceTextura < coordenadasTextura.size()) {
                    glTexCoord2f(coordenadasTextura[indiceTextura][0], coordenadasTextura[indiceTextura][1]);
                }

                // aplicar a normal correspondente
                if (indiceNormal >= 0 && indiceNormal < normais.size()) {
                    glNormal3f(normais[indiceNormal][0], normais[indiceNormal][1], normais[indiceNormal][2]);
                }

                // renderizar o vertice
                if (indiceVertice >= 0 && indiceVertice < vertices.size()) {
                    glVertex3f(vertices[indiceVertice][0], vertices[indiceVertice][1], vertices[indiceVertice][2]);
                }
            }
        }
        glEnd();
    }
    glEndList();
}

// funcao para ajustar o tamanho da janela
void redimensionarJanela(int largura, int altura) {
    glViewport(0, 0, largura, altura);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (GLfloat)largura / (GLfloat)altura, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

// funcao para desenhar o objeto
void desenharObjeto() {
    glPushMatrix();
    glColor3f(1.0, 1.0, 1.0); // definir a cor para branco
    glTranslatef(deslocamentoX, deslocamentoY, -105);
    glScalef(escala, escala, escala);
    glRotatef(angulo, 0, 1, 0);

    // vincular a textura antes de desenhar o objeto
    glBindTexture(GL_TEXTURE_2D, texturaID);

    glCallList(modeloObjeto);
    glPopMatrix();
}

// funcao de exibicao
void exibirCena(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    desenharObjeto();
    glutSwapBuffers();
}

// funcao de controle de teclado
void teclado(unsigned char tecla, int x, int y) {
    switch (tecla) {
    case 27: // esc para sair
        exit(0);
        break;
    case 'w': // deslocar para cima
        deslocamentoY += 10.0f;
        break;
    case 's': // deslocar para baixo
        deslocamentoY -= 10.0f;
        break;
    case 'a': // deslocar para a esquerda
        deslocamentoX -= 10.0f;
        break;
    case 'd': // deslocar para a direita
        deslocamentoX += 10.0f;
        break;
    case '+': // aumentar escala
        escala += 0.1f;
        break;
    case '-': // diminuir escala, mas com limite
        escala -= 0.1f;
        if (escala < 0.01f) {
            escala = 0.01f;  // limite minimo da escala
        }
        break;
    case 'r': // rotacionar no sentido horario
        angulo += 10.0f;
        break;
    case 'l': // rotacionar no sentido anti-horario
        angulo -= 10.0f;
        break;
    case '1': // ativar/desativar luz 1
        if (glIsEnabled(GL_LIGHT0)) {
            glDisable(GL_LIGHT0);
        }
        else {
            glEnable(GL_LIGHT0);
        }
        break;
    case '2': // ativar/desativar luz 2
        if (glIsEnabled(GL_LIGHT1)) {
            glDisable(GL_LIGHT1);
        }
        else {
            glEnable(GL_LIGHT1);
        }
        break;
    case '3': // ativar/desativar luz 3
        if (glIsEnabled(GL_LIGHT2)) {
            glDisable(GL_LIGHT2);
        }
        else {
            glEnable(GL_LIGHT2);
        }
        break;
    case '4': // ativar/desativar luz 4
        if (glIsEnabled(GL_LIGHT3)) {
            glDisable(GL_LIGHT3);
        }
        else {
            glEnable(GL_LIGHT3);
        }
        break;
    }
    glutPostRedisplay(); // redesenhar a cena com as novas configuracoes
}

void adicionarLuz(GLenum luzID, GLfloat* ambiente, GLfloat* difusa, GLfloat* especular, GLfloat* posicao) {
    glLightfv(luzID, GL_AMBIENT, ambiente);
    glLightfv(luzID, GL_DIFFUSE, difusa);
    glLightfv(luzID, GL_SPECULAR, especular);
    glLightfv(luzID, GL_POSITION, posicao);
    glEnable(luzID);
}

// funcao para configurar as luzes na cena
void configurarLuzes() {
    // ativar o modelo de iluminacao
    glEnable(GL_LIGHTING);

    //// ************ luz vermelha ************
    //GLfloat luzAmbienteVermelha[] = { 0.1f, 0.0f, 0.0f, 1.0f };
    //GLfloat luzDifusaVermelha[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    //GLfloat luzEspecularVermelha[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //GLfloat posicaoLuzVermelha[] = { 50.0f, 50.0f, 50.0f, 1.0f };

    //adicionarLuz(GL_LIGHT0, luzAmbienteVermelha, luzDifusaVermelha, luzEspecularVermelha, posicaoLuzVermelha);

    //// ************ luz verde ************
    //GLfloat luzAmbienteVerde[] = { 0.0f, 0.1f, 0.0f, 1.0f };
    //GLfloat luzDifusaVerde[] = { 0.0f, 1.0f, 0.0f, 1.0f };
    //GLfloat luzEspecularVerde[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //GLfloat posicaoLuzVerde[] = { -50.0f, 50.0f, 50.0f, 1.0f };

    //adicionarLuz(GL_LIGHT1, luzAmbienteVerde, luzDifusaVerde, luzEspecularVerde, posicaoLuzVerde);

    //// ************ luz azul ************
    //GLfloat luzAmbienteAzul[] = { 0.0f, 0.0f, 0.1f, 1.0f };
    //GLfloat luzDifusaAzul[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    //GLfloat luzEspecularAzul[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //GLfloat posicaoLuzAzul[] = { 0.0f, 50.0f, -50.0f, 1.0f };

    //adicionarLuz(GL_LIGHT2, luzAmbienteAzul, luzDifusaAzul, luzEspecularAzul, posicaoLuzAzul);

    // ************ luz branca ************
    GLfloat luzAmbienteBranca[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat luzDifusaBranca[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat luzEspecularBranca[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat posicaoLuzBranca[] = { 0.0f, -50.0f, 50.0f, 1.0f };

    adicionarLuz(GL_LIGHT3, luzAmbienteBranca, luzDifusaBranca, luzEspecularBranca, posicaoLuzBranca);
}

// funcao principal
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // habilitar teste de profundidade
    glutInitWindowSize(800, 450);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("carregar obj");

    glEnable(GL_DEPTH_TEST);

    configurarLuzes();

    glutReshapeFunc(redimensionarJanela);
    glutDisplayFunc(exibirCena);
    glutKeyboardFunc(teclado);

    // ativar textura
    glEnable(GL_TEXTURE_2D);

    carregarTextura("texturas/canLabel.bmp");

    carregarObjeto("data/mba1.obj");

    glutMainLoop();
    return 0;
}
