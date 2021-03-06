#include "glshaderwindow.h"
#include "joint.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QScreen>
#include <QQuaternion>
#include <QOpenGLFramebufferObjectFormat>
// Buttons/sliders for User interface:
#include <QGroupBox>
#include <QRadioButton>
#include <QSlider>
#include <QLabel>
// Layouts for User interface
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QDebug>
#include <assert.h>

#include "perlinNoise.h" // defines tables for Perlin Noise
#include "joint.h" // defines the joint class

glShaderWindow::glShaderWindow(QWindow *parent)
// Initialize obvious default values here (e.g. 0 for pointers)
    : OpenGLWindow(parent), modelMesh(0),
      m_program(0), ground_program(0), ground2_program(0), joints_program(0), compute_program(0), shadowMapGenerationProgram(0),
      g_vertices(0), g_normals(0), g_texcoords(0), g_colors(0), g_indices(0),
      g2_vertices(0), g2_normals(0), g2_texcoords(0), g2_colors(0), g2_indices(0),frame(0),rot(0),
      j_vertices(0), j_colors(0),j_indices(0),
      gpgpu_vertices(0), gpgpu_normals(0), gpgpu_texcoords(0), gpgpu_colors(0), gpgpu_indices(0),
      environmentMap(0), texture(0), permTexture(0), pixels(0), mouseButton(Qt::NoButton), auxWidget(0),
      isGPGPU(false),isSkeleton(false), hasComputeShaders(false), blinnPhong(true), transparent(true),lightning(true),bubble(false), eta(QVector2D(1.5,2.0)), lightIntensity(1.0f),refractions(5),innerRadius(0.98), shininess(50.0f), lightDistance(5.0f), groundDistance(0.78),
      shadowMap_fboId(0), shadowMap_rboId(0), shadowMap_textureId(0),animating(false),animation_time(0), fullScreenSnapshots(false),
      m_indexBuffer(QOpenGLBuffer::IndexBuffer), ground_indexBuffer(QOpenGLBuffer::IndexBuffer), ground2_indexBuffer(QOpenGLBuffer::IndexBuffer), joints_indexBuffer(QOpenGLBuffer::IndexBuffer)
{
    // Default values you might want to tinker with
    shadowMapDimension = 2048;
    // Group size for compute shaders
    compute_groupsize_x = 8;
    compute_groupsize_y = 8;

    m_fragShaderSuffix << "*.frag" << "*.fs";
    m_vertShaderSuffix << "*.vert" << "*.vs";
    m_compShaderSuffix << "*.comp" << "*.cs";
}

glShaderWindow::~glShaderWindow()
{
    if (modelMesh) delete modelMesh;
    if (m_program) {
        m_program->release();
        delete m_program;
    }
    if (ground_program) {
        ground_program->release();
        delete ground_program;
    }
    if (ground2_program) {
        ground2_program->release();
        delete ground2_program;
    }
    if(joints_program) {
      joints_program-> release();
      delete joints_program;
    }
    if (shadowMapGenerationProgram) {
        shadowMapGenerationProgram->release();
        delete shadowMapGenerationProgram;
    }
    if (compute_program) {
        compute_program->release();
        delete compute_program;
    }
    if (shadowMap_textureId) glDeleteTextures(1, &shadowMap_textureId);
    if (shadowMap_fboId) glDeleteFramebuffers(1, &shadowMap_fboId);
    if (shadowMap_rboId) glDeleteRenderbuffers(1, &shadowMap_rboId);
    if (pixels) delete [] pixels;
    m_vertexBuffer.release();
    m_vertexBuffer.destroy();
    m_indexBuffer.release();
    m_indexBuffer.destroy();
    m_colorBuffer.release();
    m_colorBuffer.destroy();
    m_normalBuffer.release();
    m_normalBuffer.destroy();
    m_texcoordBuffer.release();
    m_texcoordBuffer.destroy();
    m_vao.release();
    m_vao.destroy();
    ground_vertexBuffer.release();
    ground_vertexBuffer.destroy();
    ground_indexBuffer.release();
    ground_indexBuffer.destroy();
    ground_colorBuffer.release();
    ground_colorBuffer.destroy();
    ground_normalBuffer.release();
    ground_normalBuffer.destroy();
    ground_texcoordBuffer.release();
    ground_texcoordBuffer.destroy();
    ground_vao.release();
    ground_vao.destroy();

    ground2_vertexBuffer.release();
    ground2_vertexBuffer.destroy();
    ground2_indexBuffer.release();
    ground2_indexBuffer.destroy();
    ground2_colorBuffer.release();
    ground2_colorBuffer.destroy();
    ground2_normalBuffer.release();
    ground2_normalBuffer.destroy();
    ground2_texcoordBuffer.release();
    ground2_texcoordBuffer.destroy();
    ground2_vao.release();
    ground2_vao.destroy();
    joints_vertexBuffer.release();
    joints_vertexBuffer.destroy();
    joints_indexBuffer.release();
    joints_indexBuffer.destroy();
    joints_normalBuffer.release();
    joints_normalBuffer.destroy();
    joints_colorBuffer.release();
    joints_colorBuffer.destroy();
    joints_texcoordBuffer.release();
    joints_texcoordBuffer.destroy();
    joints_vao.release();
    joints_vao.destroy();
    if (g_vertices) delete [] g_vertices;
    if (g_colors) delete [] g_colors;
    if (g_normals) delete [] g_normals;
    if (g_indices) delete [] g_indices;
    if (g2_vertices) delete [] g2_vertices;
    if (g2_colors) delete [] g2_colors;
    if (g2_normals) delete [] g2_normals;
    if (g2_indices) delete [] g2_indices;
    if (j_vertices) delete [] j_vertices;
    if (j_colors) delete [] j_colors;
    if (j_indices) delete [] j_indices;
    if (gpgpu_vertices) delete [] gpgpu_vertices;
    if (gpgpu_colors) delete [] gpgpu_colors;
    if (gpgpu_normals) delete [] gpgpu_normals;
    if (gpgpu_indices) delete [] gpgpu_indices;
}


void glShaderWindow::openSceneFromFile() {
    QFileDialog dialog(0, "Open Scene", workingDirectory, "*.off *.obj *.ply *.3ds");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        workingDirectory = dialog.directory().path() + "/";
        modelName = dialog.selectedFiles()[0];
    }

    if (!modelName.isNull())
    {
        openScene();
        renderNow();
    }
}

void glShaderWindow::openNewTexture() {
    QFileDialog dialog(0, "Open texture image", workingDirectory + "../textures/", "*.png *.PNG *.jpg *.JPG *.tif *.TIF");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        textureName = dialog.selectedFiles()[0];
        if (!textureName.isNull()) {
            if (texture) {
                texture->release();
                texture->destroy();
                delete texture;
                texture = 0;
            }
			glActiveTexture(GL_TEXTURE0);
			// the shader wants a texture. We load one.
			texture = new QOpenGLTexture(QImage(textureName));
			if (texture) {
				texture->setWrapMode(QOpenGLTexture::Repeat);
				texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
				texture->setMagnificationFilter(QOpenGLTexture::Linear);
				texture->bind(0);
            }
        }
        renderNow();
    }
}


void glShaderWindow::openNewEnvMap() {
    QFileDialog dialog(0, "Open environment map image", workingDirectory + "../textures/", "*.png *.PNG *.jpg *.JPG");
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        envMapName= dialog.selectedFiles()[0];
        if (environmentMap) {
            environmentMap->release();
            environmentMap->destroy();
            delete environmentMap;
            environmentMap = 0;
        }
		glActiveTexture(GL_TEXTURE1);
        environmentMap = new QOpenGLTexture(QImage(envMapName).mirrored());
        if (environmentMap) {
            environmentMap->setWrapMode(QOpenGLTexture::MirroredRepeat);
            environmentMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
            environmentMap->setMagnificationFilter(QOpenGLTexture::Nearest);
            environmentMap->bind(1);
        }
        renderNow();
    }
}

void glShaderWindow::cookTorranceClicked()
{
    blinnPhong = false;
    renderNow();
}

void glShaderWindow::blinnPhongClicked()
{
    blinnPhong = true;
    renderNow();
}

void glShaderWindow::transparentClicked()
{
    transparent = true;
    bubble = false;
    renderNow();
}

void glShaderWindow::bubbleClicked(){
  bubble = true;
  transparent = true;
  renderNow();
}

void glShaderWindow::directClicked()
{
    lightning = true;
    renderNow();
}

void glShaderWindow::indirectClicked()
{
    lightning = false;
    renderNow();
}

void glShaderWindow::opaqueClicked()
{
    transparent = false;
    bubble = false;
    renderNow();
}

void glShaderWindow::updateLightIntensity(int lightSliderValue)
{
    lightIntensity = lightSliderValue / 100.0;
    renderNow();
}

void glShaderWindow::updateRefractions(int refractionsSliderValue)
{
    refractions = refractionsSliderValue ;
    renderNow();
}

void glShaderWindow::updateInnerRadius(int innerRadiusSliderValue)
{
    innerRadius = innerRadiusSliderValue/100.0 ;
    renderNow();
}

void glShaderWindow::updateShininess(int shininessSliderValue)
{
    shininess = shininessSliderValue;
    renderNow();
}

void glShaderWindow::updateEtaReal(int etaSliderValue)
{
    eta.setX(etaSliderValue/100.0);
    renderNow();
}

void glShaderWindow::updateEtaImaginary(int etaSliderValue)
{
    eta.setY(etaSliderValue/100.0);
    renderNow();
}

void glShaderWindow::updateAnimating()
{
  animating = !animating;
  setAnimating(animating);
}

void glShaderWindow::updateJoints(Joint* fath){
  /*
  std::vector<Joint*> child = root->_children;
  trimesh::point trans = trimesh::point(root->_curTx,root->_curTy,root->_curTz,0);
  std::cout<< "Translations" << root->_curTx << " ; " <<root->_curTy << " ; " << root->_curTz << std::endl;
  int curr = g2_numIndices ;
  g2_vertices[g2_numIndices]+= trans;
  for(int i = 0; i < child.size(); i ++){
    Joint* tmp = child[i];
    //trimesh::matrix T = trimesh::matrix(tmp->_curTx,tmp->_curTy,tmp->_curTz,1);
    g2_numIndices++;
    updateJoints(tmp);
  }*/
}

QWidget *glShaderWindow::makeAuxWindow()
{
    if (auxWidget)
        if (auxWidget->isVisible()) return auxWidget;
    auxWidget = new QWidget;

    QVBoxLayout *outer = new QVBoxLayout;
    QHBoxLayout *buttons = new QHBoxLayout;

    QGroupBox *groupBox = new QGroupBox("Specular Model selection");
    QRadioButton *radio1 = new QRadioButton("Blinn-Phong");
    QRadioButton *radio2 = new QRadioButton("Cook-Torrance");
    if (blinnPhong) radio1->setChecked(true);
    else radio1->setChecked(true);
    connect(radio1, SIGNAL(clicked()), this, SLOT(blinnPhongClicked()));
    connect(radio2, SIGNAL(clicked()), this, SLOT(cookTorranceClicked()));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(radio1);
    vbox->addWidget(radio2);
    groupBox->setLayout(vbox);
    buttons->addWidget(groupBox);

    QGroupBox *groupBox2 = new QGroupBox("Surface:");
    QRadioButton *transparent1 = new QRadioButton("&Transparent");
    QRadioButton *transparent2 = new QRadioButton("&Opaque");
    QRadioButton *bubble1 = new QRadioButton("&Bulle");
    if (transparent) transparent1->setChecked(true);
    else transparent2->setChecked(true);
    if(bubble) bubble1->setChecked(true);
    connect(transparent1, SIGNAL(clicked()), this, SLOT(transparentClicked()));
    connect(transparent2, SIGNAL(clicked()), this, SLOT(opaqueClicked()));
    connect(bubble1,SIGNAL(clicked()),this,SLOT(bubbleClicked()));
    QVBoxLayout *vbox2 = new QVBoxLayout;
    vbox2->addWidget(transparent1);
    vbox2->addWidget(transparent2);
    vbox2->addWidget(bubble1);
    groupBox2->setLayout(vbox2);
    buttons->addWidget(groupBox2);
    outer->addLayout(buttons);


    QGroupBox *groupBox3 = new QGroupBox("Lightning:");
    QRadioButton *lightning1 = new QRadioButton("&Direct");
    QRadioButton *lightning2 = new QRadioButton("&Indirect");
    if (lightning) lightning1->setChecked(true);
    else lightning2->setChecked(true);
    connect(lightning1, SIGNAL(clicked()), this, SLOT(directClicked()));
    connect(lightning2, SIGNAL(clicked()), this, SLOT(indirectClicked()));
    QVBoxLayout *vbox3 = new QVBoxLayout;
    vbox3->addWidget(lightning1);
    vbox3->addWidget(lightning2);
    groupBox3->setLayout(vbox3);
    buttons->addWidget(groupBox3);
    outer->addLayout(buttons);

    // number of refractions
    QSlider* refractionsSlider = new QSlider(Qt::Horizontal);
    refractionsSlider->setTickPosition(QSlider::TicksBelow);
    refractionsSlider->setMinimum(0);
    refractionsSlider->setMaximum(10);
    refractionsSlider->setSliderPosition(refractions);
    connect(refractionsSlider,SIGNAL(valueChanged(int)),this,SLOT(updateRefractions(int)));
    QLabel* refractionsLabel = new QLabel("Number of refractions / bounces = ");
    QLabel* refractionsLabelValue = new QLabel();
    refractionsLabelValue->setNum(refractions);
    connect(refractionsSlider,SIGNAL(valueChanged(int)),refractionsLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxRefractions = new QHBoxLayout;
    hboxRefractions->addWidget(refractionsLabel);
    hboxRefractions->addWidget(refractionsLabelValue);
    outer->addLayout(hboxRefractions);
    outer->addWidget(refractionsSlider);


    // inner radius slider for the bubbles
    QSlider* innerRadiusSlider = new QSlider(Qt::Horizontal);
    innerRadiusSlider->setTickPosition(QSlider::TicksBelow);
    innerRadiusSlider->setMinimum(25);
    innerRadiusSlider->setMaximum(98);
    innerRadiusSlider->setSliderPosition(innerRadius*100);
    connect(innerRadiusSlider,SIGNAL(valueChanged(int)),this,SLOT(updateInnerRadius(int)));
    QLabel* innerRadiusLabel = new QLabel("Radius of the inner bubble  in percentage of outer bubble = ");
    QLabel* innerRadiusLabelValue = new QLabel();
    innerRadiusLabelValue->setNum(100*innerRadius);
    connect(innerRadiusSlider,SIGNAL(valueChanged(int)),innerRadiusLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxInnerRadius = new QHBoxLayout;
    hboxInnerRadius->addWidget(innerRadiusLabel);
    hboxInnerRadius->addWidget(innerRadiusLabelValue);
    outer->addLayout(hboxInnerRadius);
    outer->addWidget(innerRadiusSlider);

    // light source intensity
    QSlider* lightSlider = new QSlider(Qt::Horizontal);
    lightSlider->setTickPosition(QSlider::TicksBelow);
    lightSlider->setMinimum(0);
    lightSlider->setMaximum(200);
    lightSlider->setSliderPosition(100*lightIntensity);
    connect(lightSlider,SIGNAL(valueChanged(int)),this,SLOT(updateLightIntensity(int)));
    QLabel* lightLabel = new QLabel("Light intensity = ");
    QLabel* lightLabelValue = new QLabel();
    lightLabelValue->setNum(100 * lightIntensity);
    connect(lightSlider,SIGNAL(valueChanged(int)),lightLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxLight = new QHBoxLayout;
    hboxLight->addWidget(lightLabel);
    hboxLight->addWidget(lightLabelValue);
    outer->addLayout(hboxLight);
    outer->addWidget(lightSlider);

    // Phong shininess slider
    QSlider* shininessSlider = new QSlider(Qt::Horizontal);
    shininessSlider->setTickPosition(QSlider::TicksBelow);
    shininessSlider->setMinimum(0);
    shininessSlider->setMaximum(200);
    shininessSlider->setSliderPosition(shininess);
    connect(shininessSlider,SIGNAL(valueChanged(int)),this,SLOT(updateShininess(int)));
    QLabel* shininessLabel = new QLabel("Phong exponent = ");
    QLabel* shininessLabelValue = new QLabel();
    shininessLabelValue->setNum(shininess);
    connect(shininessSlider,SIGNAL(valueChanged(int)),shininessLabelValue,SLOT(setNum(int)));
    QHBoxLayout *hboxShininess = new QHBoxLayout;
    hboxShininess->addWidget(shininessLabel);
    hboxShininess->addWidget(shininessLabelValue);
    outer->addLayout(hboxShininess);
    outer->addWidget(shininessSlider);

    // Real part of eta slider
    QSlider* etaSliderReal = new QSlider(Qt::Horizontal);
    etaSliderReal->setTickPosition(QSlider::TicksBelow);
    etaSliderReal->setTickInterval(100);
    etaSliderReal->setMinimum(0);
    etaSliderReal->setMaximum(500);
    etaSliderReal->setSliderPosition(eta.x()*100);
    connect(etaSliderReal,SIGNAL(valueChanged(int)),this,SLOT(updateEtaReal(int)));
    QLabel* etaLabelReal = new QLabel("Real part of eta (index of refraction) * 100 =");
    QLabel* etaLabelValueReal = new QLabel();
    etaLabelValueReal->setNum(eta.x() * 100);
    connect(etaSliderReal,SIGNAL(valueChanged(int)),etaLabelValueReal,SLOT(setNum(int)));
    QHBoxLayout *hboxEtaReal= new QHBoxLayout;
    hboxEtaReal->addWidget(etaLabelReal);
    hboxEtaReal->addWidget(etaLabelValueReal);
    outer->addLayout(hboxEtaReal);
    outer->addWidget(etaSliderReal);

    // Imaginary part of eta slider
    QSlider* etaSliderI = new QSlider(Qt::Horizontal);
    etaSliderI->setTickPosition(QSlider::TicksBelow);
    etaSliderI->setTickInterval(100);
    etaSliderI->setMinimum(0);
    etaSliderI->setMaximum(500);
    etaSliderI->setSliderPosition(eta.y()*100);
    connect(etaSliderI,SIGNAL(valueChanged(int)),this,SLOT(updateEtaImaginary(int)));
    QLabel* etaLabelI = new QLabel("Imaginary part of eta (index of refraction) * 100 =");
    QLabel* etaLabelValueI = new QLabel();
    etaLabelValueI->setNum(eta.y() * 100);
    connect(etaSliderI,SIGNAL(valueChanged(int)),etaLabelValueI,SLOT(setNum(int)));
    QHBoxLayout *hboxEtaI= new QHBoxLayout;
    hboxEtaI->addWidget(etaLabelI);
    hboxEtaI->addWidget(etaLabelValueI);
    outer->addLayout(hboxEtaI);
    outer->addWidget(etaSliderI);

    auxWidget->setLayout(outer);
    return auxWidget;
}

void glShaderWindow::createSSBO()
{
#ifndef __APPLE__
	glGenBuffers(4, ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
    // TODO: test if 4 float alignment works better
    glBufferData(GL_SHADER_STORAGE_BUFFER, modelMesh->vertices.size() * sizeof(trimesh::point), &(modelMesh->vertices.front()), GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, modelMesh->normals.size() * sizeof(trimesh::vec), &(modelMesh->normals.front()), GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[2]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, modelMesh->colors.size() * sizeof(trimesh::Color), &(modelMesh->colors.front()), GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[3]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, modelMesh->faces.size() * 3 * sizeof(int), &(modelMesh->faces.front()), GL_STATIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    compute_program->bind();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[1]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo[2]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo[3]);
#endif
}


int glShaderWindow::treeCount(Joint* root, int count){
  std::vector<Joint*> child = root->_children;
  for(int i = 0; i < child.size(); i ++){
       count += treeCount(child[i],0);
  }
  return count+1;
}

void glShaderWindow::treeConstruct(Joint* fath){
  std::vector<Joint*> child = fath->_children;
  int curr = g2_numIndices;
  QVector4D father = QVector4D();
  father.setX((g2_vertices[curr])[0]);
  father.setY((g2_vertices[curr])[1]);
  father.setZ((g2_vertices[curr])[2]);
  father.setW(1);
  QVector4D vec;
  for(int i = 0; i < child.size(); i ++){
    Joint* tmp = child[i];
    // Remplir les buffer avec les valeurs de base :
    //trn[g2_numIndices+1]+=trimesh::point(tmp->_curTx,tmp->_curTy,tmp->_curTz,0);
    g2_vertices[g2_numIndices+1]=trimesh::point((g2_vertices[curr])[0],(g2_vertices[curr])[1],(g2_vertices[curr])[2],1);
    g2_colors[g2_numIndices+1] = trimesh::point(1, 0, 0, 1);
    g2_normals[g2_numIndices+1]=trimesh::point(0,1,0,0);
    g2_texcoords[g2_numIndices+1]=trimesh::vec2(0,0);
    g2_indices[(g2_numIndices)*2] = curr;
    g2_indices[(g2_numIndices)*2+1] = g2_numIndices+1;
    g2_numIndices++;

    // Partie Matrices de Rotation pour animation
    QMatrix4x4 rX;
    rX.rotate(tmp->_curRx,1,0,0);
    QMatrix4x4 rY;
    rY.rotate(tmp->_curRy,0,1,0);
    QMatrix4x4 rZ;
    rZ.rotate(tmp->_curRz,0,0,1);
    QMatrix4x4 M;
  //  std::cout << " angle curRx : "<< tmp->_curRx  << "  from  : " << tmp->_name <<  std::endl;
    switch(fath->_rorder){
      case 0 :
        M = rX*rY*rZ;
      break;
      case 1 :
        M = rY*rZ*rX;
      break;
      case 2 :
        M = rZ*rX*rY;
      break ;
      case 3 :
        M = rX*rZ*rY;
      break ;
      case 4 :
        M = rY*rX*rZ;
      break;
      case 5 :
        M=rZ*rY*rX;
      break;
      default :
        std::cout<< " Should not be there dude " << std::endl;
      break;
    }
    // Recuperation de l'offset pour pouvoir faire la transformation du point
    QMatrix4x4 T;
    T.translate(tmp->_offX+tmp->_curTx,tmp->_offY+tmp->_curTy,tmp->_offZ+tmp->_curTz);
    M = T*M;
    rot[g2_numIndices]= rot[curr]*M;// multiplication par la transfo du père

    // Ajout du vertice dans la liste
    QVector4D origin = QVector4D(0,0,0,1);
    vec = rot[g2_numIndices] * origin;
    g2_vertices[g2_numIndices] = trimesh::point(vec.x(),vec.y(),vec.z(),vec.w());
    // On réitère sur les fils
    treeConstruct(tmp);
  }
}

void glShaderWindow::bindSceneToProgram()
{
    // Now create the VAO for the model
    m_vao.bind();
    // If we're doing GPGPU, the VAO is very simple: 4 vertices for a large square covering the screen.
    if (isGPGPU) {
        m_numFaces = 2;
        // Allocate and fill only once
        if (gpgpu_vertices == 0)
        {
            gpgpu_vertices = new trimesh::point[4];
            if (gpgpu_normals == 0) gpgpu_normals = new trimesh::vec[4];
            if (gpgpu_colors == 0) gpgpu_colors = new trimesh::point[4];
            if (gpgpu_texcoords == 0) gpgpu_texcoords = new trimesh::vec2[4];
            if (gpgpu_indices == 0) gpgpu_indices = new int[6];
            gpgpu_vertices[0] = trimesh::point(-1, -1, 0, 1);
            gpgpu_vertices[1] = trimesh::point(-1, 1, 0, 1);
            gpgpu_vertices[2] = trimesh::point(1, -1, 0, 1);
            gpgpu_vertices[3] = trimesh::point(1, 1, 0, 1);
            for (int i = 0; i < 4; i++) {
                gpgpu_normals[i] = trimesh::point(0, 0, -1, 0);
                gpgpu_colors[i] = trimesh::point(0, 0, 1, 1);
            }
            gpgpu_texcoords[0] = trimesh::vec2(0, 0);
            gpgpu_texcoords[1] = trimesh::vec2(0, 1);
            gpgpu_texcoords[2] = trimesh::vec2(1, 0);
            gpgpu_texcoords[3] = trimesh::vec2(1, 1);
            gpgpu_indices[0] = 0;
            gpgpu_indices[1] = 2;
            gpgpu_indices[2] = 1;
            gpgpu_indices[3] = 1;
            gpgpu_indices[4] = 2;
            gpgpu_indices[5] = 3;
        }
    } else m_numFaces = modelMesh->faces.size();

    m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_vertexBuffer.bind();
    if (!isGPGPU) m_vertexBuffer.allocate(&(modelMesh->vertices.front()), modelMesh->vertices.size() * sizeof(trimesh::point));
    else m_vertexBuffer.allocate(gpgpu_vertices, 4 * sizeof(trimesh::point));

    m_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_indexBuffer.bind();
    if (!isGPGPU) m_indexBuffer.allocate(&(modelMesh->faces.front()), m_numFaces * 3 * sizeof(int));
    else m_indexBuffer.allocate(gpgpu_indices, m_numFaces * 3 * sizeof(int));

    if (modelMesh->colors.size() > 0) {
        m_colorBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_colorBuffer.bind();
        if (!isGPGPU) m_colorBuffer.allocate(&(modelMesh->colors.front()), modelMesh->colors.size() * sizeof(trimesh::Color));
        else m_colorBuffer.allocate(gpgpu_colors, 4 * sizeof(trimesh::Color));
    }

    m_normalBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_normalBuffer.bind();
    if (!isGPGPU) m_normalBuffer.allocate(&(modelMesh->normals.front()), modelMesh->normals.size() * sizeof(trimesh::vec));
    else m_normalBuffer.allocate(gpgpu_normals, 4 * sizeof(trimesh::vec));

    if ((modelMesh->texcoords.size() > 0) || isGPGPU) {
        m_texcoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_texcoordBuffer.bind();
        if (!isGPGPU) m_texcoordBuffer.allocate(&(modelMesh->texcoords.front()), modelMesh->texcoords.size() * sizeof(trimesh::vec2));
        else m_texcoordBuffer.allocate(gpgpu_texcoords, 4 * sizeof(trimesh::vec2));
    }
    m_program->bind();
    // Enable the "vertex" attribute to bind it to our vertex buffer
    m_vertexBuffer.bind();
    m_program->setAttributeBuffer( "vertex", GL_FLOAT, 0, 4 );
    m_program->enableAttributeArray( "vertex" );

    // Enable the "color" attribute to bind it to our colors buffer
    if (modelMesh->colors.size() > 0) {
        m_colorBuffer.bind();
        m_program->setAttributeBuffer( "color", GL_FLOAT, 0, 4 );
        m_program->enableAttributeArray( "color" );
        m_program->setUniformValue("noColor", false);
    } else {
        m_program->setUniformValue("noColor", true);
    }
    m_normalBuffer.bind();
    m_program->setAttributeBuffer( "normal", GL_FLOAT, 0, 4 );
    m_program->enableAttributeArray( "normal" );

    if ((modelMesh->texcoords.size() > 0) || isGPGPU){
        m_texcoordBuffer.bind();
        m_program->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
        m_program->enableAttributeArray( "texcoords" );
    }
    m_program->release();
    shadowMapGenerationProgram->bind();
    // Enable the "vertex" attribute to bind it to our vertex buffer
    m_vertexBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "vertex", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "vertex" );
    if (modelMesh->colors.size() > 0) {
        m_colorBuffer.bind();
        shadowMapGenerationProgram->setAttributeBuffer( "color", GL_FLOAT, 0, 4 );
        shadowMapGenerationProgram->enableAttributeArray( "color" );
    }
    m_normalBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "normal", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "normal" );
    if (modelMesh->texcoords.size() > 0) {
        m_texcoordBuffer.bind();
        shadowMapGenerationProgram->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
        shadowMapGenerationProgram->enableAttributeArray( "texcoords" );
    }
    shadowMapGenerationProgram->release();
    m_vao.release();

    // Bind ground VAO to ground program as well
    // We create a VAO for the ground from scratch
    ground_vao.bind();
    ground_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_vertexBuffer.bind();
    trimesh::point center = modelMesh->bsphere.center;
    float radius = modelMesh->bsphere.r;

    int numR = 10;
    int numTh = 20;
    g_numPoints = numR * numTh;
    // Allocate once, fill in for every new model.
    if (g_vertices == 0) g_vertices = new trimesh::point[g_numPoints];
    if (g_normals == 0) g_normals = new trimesh::vec[g_numPoints];
    if (g_colors == 0) g_colors = new trimesh::point[g_numPoints];
    if (g_texcoords == 0) g_texcoords = new trimesh::vec2[g_numPoints];
    if (g_indices == 0) g_indices = new int[6 * g_numPoints];
    for (int i = 0; i < numR; i++) {
        for (int j = 0; j < numTh; j++) {
            int p = i + j * numR;
            g_normals[p] = trimesh::point(0, 1, 0, 0);
            g_colors[p] = trimesh::point(0.6, 0.85, 0.9, 1);
            float theta = (float)j * 2 * M_PI / numTh;
            float rad =  5.0 * radius * (float) i / numR;
            g_vertices[p] = center + trimesh::point(rad * cos(theta), - groundDistance * radius, rad * sin(theta), 0);
            rad =  5.0 * (float) i / numR;
            g_texcoords[p] = trimesh::vec2(rad * cos(theta), rad * sin(theta));
        }
    }
    ground_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_vertexBuffer.bind();
    ground_vertexBuffer.allocate(g_vertices, g_numPoints * sizeof(trimesh::point));
    ground_normalBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_normalBuffer.bind();
    ground_normalBuffer.allocate(g_normals, g_numPoints * sizeof(trimesh::vec));
    ground_colorBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_colorBuffer.bind();
    ground_colorBuffer.allocate(g_colors, g_numPoints * sizeof(trimesh::point));
    ground_texcoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_texcoordBuffer.bind();
    ground_texcoordBuffer.allocate(g_texcoords, g_numPoints * sizeof(trimesh::vec2));

    g_numIndices = 0;
    for (int i = 0; i < numR - 1; i++) {
        for (int j = 0; j < numTh; j++) {
            int j_1 = (j + 1) % numTh;
            g_indices[g_numIndices++] = i + j * numR;
            g_indices[g_numIndices++] = i + 1 + j_1 * numR;
            g_indices[g_numIndices++] = i + 1 + j * numR;
            g_indices[g_numIndices++] = i + j * numR;
            g_indices[g_numIndices++] = i + j_1 * numR;
            g_indices[g_numIndices++] = i + 1 + j_1 * numR;
        }

    }
    ground_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground_indexBuffer.bind();
    ground_indexBuffer.allocate(g_indices, g_numIndices * sizeof(int));

    ground_program->bind();
    ground_vertexBuffer.bind();
    ground_program->setAttributeBuffer( "vertex", GL_FLOAT, 0, 4 );
    ground_program->enableAttributeArray( "vertex" );
    ground_colorBuffer.bind();
    ground_program->setAttributeBuffer( "color", GL_FLOAT, 0, 4 );
    ground_program->enableAttributeArray( "color" );
    ground_normalBuffer.bind();
    ground_program->setAttributeBuffer( "normal", GL_FLOAT, 0, 4 );
    ground_program->enableAttributeArray( "normal" );
    ground_program->setUniformValue("noColor", false);
    ground_texcoordBuffer.bind();
    ground_program->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
    ground_program->enableAttributeArray( "texcoords" );
    ground_program->release();
    // Also bind the ground to the shadow mapping program:
    shadowMapGenerationProgram->bind();
    ground_vertexBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "vertex", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "vertex" );
    shadowMapGenerationProgram->release();
    ground_colorBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "color", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "color" );
    ground_normalBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "normal", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "normal" );
    ground_texcoordBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
    shadowMapGenerationProgram->enableAttributeArray( "texcoords" );
    ground_program->release();
    ground_vao.release();



// Bind ground2 VAO to ground2 program as well
    // We create a VAO for the ground2 from scratch
    ground2_vao.bind();
    ground2_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground2_vertexBuffer.bind();
    trimesh::point center2 = modelMesh->bsphere.center+ trimesh::point(2,2,2,0);
    float radius2 = modelMesh->bsphere.r;
    // Allocate once, fill in for every new model.
    if (!animating){
      root = Joint::createFromFile("./viewer/animation/walk1.bvh");
    }

    // Algorithme de parcours :
    // Tracer de current vers chacun des fils
    // Itérer sur les fils
    g2_numPoints=treeCount(root,0);
    if (g2_vertices == 0) g2_vertices = new trimesh::point[g2_numPoints];
    if (g2_normals == 0) g2_normals = new trimesh::vec[g2_numPoints];
    if (g2_colors == 0) g2_colors = new trimesh::point[g2_numPoints];
    if (g2_texcoords == 0) g2_texcoords = new trimesh::vec2[g2_numPoints];
    if (g2_indices == 0) g2_indices = new int[(g2_numPoints-1)*2];
    if(rot==0) rot = new QMatrix4x4[g2_numPoints];
    g2_numIndices=0;
    QMatrix4x4 rX;
    rX.rotate(root->_curRx,1,0,0);
    QMatrix4x4 rY;
    rY.rotate(root->_curRy,0,1,0);
    QMatrix4x4 rZ;
    rZ.rotate(root->_curRz,0,0,1);
    QMatrix4x4 M;
    switch(root->_rorder){
      case 0 :
        M = rX*rY*rZ;
      break;
      case 1 :
        M = rY*rZ*rX;
      break;
      case 2 :
        M = rZ*rX*rY;
      break ;
      case 3 :
        M = rX*rZ*rY;
      break ;
      case 4 :
        M = rY*rX*rZ;
      break;
      case 5 :
        M=rZ*rY*rX;
      break;
      default :
        std::cout<< " Should not be there dude " << std::endl;
      break;
    }
    QMatrix4x4 T;
    T.translate(root->_offX+root->_curTx,root->_offY+root->_curTy);
    M = T*M;
    rot[0]=M;
    g2_vertices[0]=trimesh::point(root->_offX,root->_offY,root->_offZ,1);
    g2_colors[0] = trimesh::point(1.0, 0.0, 0.0, 1);
    g2_normals[0]= trimesh::point(0,1,0,0);
    g2_texcoords[0]= trimesh::vec2(0,0);
    treeConstruct(root);
    ground2_vertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    ground2_vertexBuffer.bind();
    ground2_vertexBuffer.allocate(g2_vertices, g2_numPoints * sizeof(trimesh::point));
    ground2_normalBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground2_normalBuffer.bind();
    ground2_normalBuffer.allocate(g2_normals, g2_numPoints * sizeof(trimesh::vec));
    ground2_colorBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground2_colorBuffer.bind();
    ground2_colorBuffer.allocate(g2_colors, g2_numPoints * sizeof(trimesh::point));
    ground2_texcoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground2_texcoordBuffer.bind();
    ground2_texcoordBuffer.allocate(g2_texcoords, g2_numPoints * sizeof(trimesh::vec2));
    ground2_indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    ground2_indexBuffer.bind();
    ground2_indexBuffer.allocate(g2_indices, (g2_numPoints-1)*2 * sizeof(int));

    ground2_program->bind();
    ground2_vertexBuffer.bind();
    ground2_program->setAttributeBuffer( "vertex", GL_FLOAT, 0, 4 );
    ground2_program->enableAttributeArray( "vertex" );
    ground2_colorBuffer.bind();
    ground2_program->setAttributeBuffer( "color", GL_FLOAT, 0, 4 );
    ground2_program->enableAttributeArray( "color" );
    ground2_normalBuffer.bind();
    ground2_program->setAttributeBuffer( "normal", GL_FLOAT, 0, 4 );
    ground2_program->enableAttributeArray( "normal" );
    ground2_program->setUniformValue("noColor", false);
    ground2_texcoordBuffer.bind();
    ground2_program->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
    ground2_program->enableAttributeArray( "texcoords" );
    ground2_program->release();
    // Also bind the ground2 to the shadow mapping program:
    shadowMapGenerationProgram->bind();
    ground2_vertexBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "vertex", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "vertex" );
    shadowMapGenerationProgram->release();
    ground2_colorBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "color", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "color" );
    ground2_normalBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "normal", GL_FLOAT, 0, 4 );
    shadowMapGenerationProgram->enableAttributeArray( "normal" );
    ground2_texcoordBuffer.bind();
    shadowMapGenerationProgram->setAttributeBuffer( "texcoords", GL_FLOAT, 0, 2 );
    shadowMapGenerationProgram->enableAttributeArray( "texcoords" );
    ground2_program->release();
    ground2_vao.release();
}

void glShaderWindow::initializeTransformForScene()
{
    // Set standard transformation and light source
    float radius = modelMesh->bsphere.r;
    m_perspective.setToIdentity();
    m_perspective.perspective(45, (float)width()/height(), 0.1 * radius, 20 * radius);
    QVector3D eye = m_center + 2 * radius * QVector3D(0,0,1);
    m_matrix[0].setToIdentity();
    m_matrix[1].setToIdentity();
    m_matrix[2].setToIdentity();
    m_matrix[0].lookAt(eye, m_center, QVector3D(0,1,0));
    m_matrix[1].translate(-m_center);
}

void glShaderWindow::openScene()
{
    if (modelMesh) {
        delete(modelMesh);
        m_vertexBuffer.release();
        m_indexBuffer.release();
        m_colorBuffer.release();
        m_normalBuffer.release();
        m_texcoordBuffer.release();
        m_vao.release();
    }

    modelMesh = trimesh::TriMesh::read(qPrintable(modelName));
    if (!modelMesh) {
        QMessageBox::warning(0, tr("qViewer"),
                             tr("Could not load file ") + modelName, QMessageBox::Ok);
        openSceneFromFile();
    }
    modelMesh->need_bsphere();
    modelMesh->need_bbox();
    modelMesh->need_normals();
    modelMesh->need_faces();
    m_center = QVector3D(modelMesh->bsphere.center[0],
            modelMesh->bsphere.center[1],
            modelMesh->bsphere.center[2]);

    m_bbmin = QVector3D(modelMesh->bbox.min[0],
            modelMesh->bbox.min[1],
            modelMesh->bbox.min[2]);
    m_bbmax = QVector3D(modelMesh->bbox.max[0],
            modelMesh->bbox.max[1],
            modelMesh->bbox.max[2]);
	if (compute_program) {
        createSSBO();
    }
    bindSceneToProgram();
    initializeTransformForScene();
}

void glShaderWindow::saveScene()
{
    QFileDialog dialog(0, "Save current scene", workingDirectory,
    "*.ply *.ray *.obj *.off *.sm *.stl *.cc *.dae *.c++ *.C *.c++");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        workingDirectory = dialog.directory().path();
        filename = dialog.selectedFiles()[0];
    }
    if (!filename.isNull()) {
        if (!modelMesh->write(qPrintable(filename))) {
            QMessageBox::warning(0, tr("qViewer"),
                tr("Could not save file: ") + filename, QMessageBox::Ok);
        }
    }
}

void glShaderWindow::toggleFullScreen()
{
    fullScreenSnapshots = !fullScreenSnapshots;
}

void glShaderWindow::saveScreenshot()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap pixmap;
    if (screen) {
        if (fullScreenSnapshots) pixmap = screen->grabWindow(winId());
        else pixmap = screen->grabWindow(winId(), parent()->x(), parent()->y(), parent()->width(), parent()->height());
        // This grabs the window and the control panel
        // To get the window only:
        // pixmap = screen->grabWindow(winId(), parent()->x() + x(), parent()->y() + y(), width(), height());
    }
    QFileDialog dialog(0, "Save current picture", workingDirectory, "*.png *.jpg");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted) {
        filename = dialog.selectedFiles()[0];
        if (!pixmap.save(filename)) {
            QMessageBox::warning(0, tr("qViewer"),
                tr("Could not save picture file: ") + filename, QMessageBox::Ok);
        }
    }
}

void glShaderWindow::setWindowSize(const QString& size)
{
    QStringList dims = size.split("x");
    parent()->resize(parent()->width() - width() + dims[0].toInt(), parent()->height() - height() + dims[1].toInt());
    resize(dims[0].toInt(), dims[1].toInt());
    renderNow();
}

void glShaderWindow::setShader(const QString& shader)
{
    // Prepare a complete shader program...$
	  QString shaderPath = workingDirectory + "../shaders/";
    QDir shadersDir = QDir(shaderPath);
    QString shader2 = shader + "*";
    QStringList shaders = shadersDir.entryList(QStringList(shader2));
    QString vertexShader;
    QString fragmentShader;
    QString computeShader;
    isGPGPU = shader.contains("gpgpu", Qt::CaseInsensitive);
    isFullrt = shader.contains("fullrt", Qt::CaseInsensitive);
    isSkeleton = shader.contains("animation",Qt::CaseInsensitive);
    // TODO : gérer le shader pour lancer le compute du squelette ici
    foreach (const QString &str, shaders) {
        QString suffix = str.right(str.size() - str.lastIndexOf("."));
        if (m_vertShaderSuffix.filter(suffix).size() > 0) {
            vertexShader = shaderPath + str;
        }
        if (m_fragShaderSuffix.filter(suffix).size() > 0) {
            fragmentShader = shaderPath + str;
        }
        if (m_compShaderSuffix.filter(suffix).size() > 0) {
            computeShader = shaderPath + str;
        }
    }
    if(!isSkeleton){
      m_program = prepareShaderProgram(vertexShader, fragmentShader);
      if (computeShader.length() > 0) {
    	   compute_program = prepareComputeProgram(computeShader);
         if (compute_program) createSSBO();
	      } else if (compute_program) {
          compute_program->release();
          delete compute_program;
          compute_program = 0;
          hasComputeShaders = false;
        }
      }else{
        joints_program=prepareShaderProgram(vertexShader,fragmentShader);
        if (computeShader.length() > 0) {
    	   compute_program = prepareComputeProgram(computeShader);
         if (compute_program) createSSBO();
	      } else if (compute_program) {
          compute_program->release();
          delete compute_program;
          compute_program = 0;
          hasComputeShaders = false;
        }
      }
    bindSceneToProgram();
    loadTexturesForShaders();
    if(animating && isFullrt ) animation_time = (animation_time-1)%4;
    renderNow();
}

void glShaderWindow::loadTexturesForShaders() {
    m_program->bind();
    // Erase all existing textures:
    if (texture) {
        texture->release();
        texture->destroy();
        delete texture;
        texture = 0;
    }
    if (permTexture) {
        permTexture->release();
        permTexture->destroy();
        delete permTexture;
        permTexture = 0;
    }
    if (environmentMap) {
        environmentMap->release();
        environmentMap->destroy();
        delete environmentMap;
        environmentMap = 0;
    }
    if (computeResult) {
        computeResult->release();
        computeResult->destroy();
        delete computeResult;
        computeResult = 0;
    }

    // if(!texture){
      // textureName = "bricks.png";
    // }
	// Load textures as required by the shader.
  // if ((m_program->uniformLocation("colorTexture") != -1) ) {
	if ((m_program->uniformLocation("colorTexture") != -1) || (ground_program->uniformLocation("colorTexture") != -1) || (ground2_program->uniformLocation("colorTexture") != -1) || (joints_program->uniformLocation("colorTexture") != -1)) {
		glActiveTexture(GL_TEXTURE0);
        // the shader wants a texture. We load one.
        texture = new QOpenGLTexture(QImage(textureName));
        if (texture) {
            texture->setWrapMode(QOpenGLTexture::Repeat);
            texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
            texture->setMagnificationFilter(QOpenGLTexture::Linear);
            texture->bind(0);
        }
    }
    if (m_program->uniformLocation("envMap") != -1) {
		glActiveTexture(GL_TEXTURE1);
        // the shader wants an environment map, we load one.
        environmentMap = new QOpenGLTexture(QImage(envMapName).mirrored());
        if (environmentMap) {
            environmentMap->setWrapMode(QOpenGLTexture::MirroredRepeat);
            environmentMap->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
            environmentMap->setMagnificationFilter(QOpenGLTexture::Linear);
            environmentMap->bind(1);
        }
    } else {
        // for Perlin noise
		glActiveTexture(GL_TEXTURE1);
        if (m_program->uniformLocation("permTexture") != -1) {
            permTexture = new QOpenGLTexture(QImage(pixels, 256, 256, QImage::Format_RGBA8888));
            if (permTexture) {
                permTexture->setWrapMode(QOpenGLTexture::MirroredRepeat);
                permTexture->setMinificationFilter(QOpenGLTexture::Nearest);
                permTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
                permTexture->bind(1);
            }
        }
    }
    if (hasComputeShaders) {
        // We bind the texture generated to texture unit 2 (0 is for the texture, 1 for the env map)
        // 2 is also used by the shadow map. If you need both, move one to texture unit 3.
		glActiveTexture(GL_TEXTURE2);
        computeResult = new QOpenGLTexture(QOpenGLTexture::Target2D);
        if (computeResult) {
        	computeResult->create();
            computeResult->setFormat(QOpenGLTexture::RGBA32F);
            computeResult->setSize(width(), height());
            computeResult->setWrapMode(QOpenGLTexture::MirroredRepeat);
            computeResult->setMinificationFilter(QOpenGLTexture::Nearest);
            computeResult->setMagnificationFilter(QOpenGLTexture::Nearest);
            computeResult->allocateStorage();
            computeResult->bind(2);
        }
    } else if ((ground_program->uniformLocation("shadowMap") != -1)
        || (ground2_program->uniformLocation("shadowMap") != -1)
    		|| (m_program->uniformLocation("shadowMap") != -1)
        || (joints_program->uniformLocation("shadowMap") != -1) ){
    	// without Qt functions this time
		glActiveTexture(GL_TEXTURE2);
		if (shadowMap_textureId == 0) glGenTextures(1, &shadowMap_textureId);
		glBindTexture(GL_TEXTURE_2D, shadowMap_textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap generation included in OpenGL v1.4
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapDimension, shadowMapDimension, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        // create a framebuffer object, you need to delete them when program exits.
        if (shadowMap_fboId == 0) glGenFramebuffers(1, &shadowMap_fboId);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMap_fboId);

        // create a renderbuffer object to store depth info
        // NOTE: A depth renderable image should be attached the FBO for depth test.
        // If we don't attach a depth renderable image to the FBO, then
        // the rendering output will be corrupted because of missing depth test.
        // If you also need stencil test for your rendering, then you must
        // attach additional image to the stencil attachement point, too.
        if (shadowMap_rboId == 0) glGenRenderbuffers(1, &shadowMap_rboId);
        glBindRenderbuffer(GL_RENDERBUFFER, shadowMap_rboId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, shadowMapDimension, shadowMapDimension);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // attach a texture to FBO depth attachement point
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap_textureId, 0);

        // attach a renderbuffer to depth attachment point
        //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER_EXT, rboId);

        //@@ disable color buffer if you don't attach any color buffer image,
        //@@ for example, rendering depth buffer only to a texture.
        //@@ Otherwise, glCheckFramebufferStatusEXT will not be complete.
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, shadowMap_textureId);
	}
    m_program->release();
}

void glShaderWindow::initialize()
{
    // Debug: which OpenGL version are we running? Must be >= 3.2 for shaders,
    // >= 4.3 for compute shaders.
    qDebug("OpenGL initialized: version: %s GLSL: %s", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    // Set the clear color to black
    glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
    glEnable (GL_CULL_FACE); // cull face
    glCullFace (GL_BACK); // cull back face
    glFrontFace (GL_CCW); // GL_CCW for counter clock-wise
    glEnable (GL_DEPTH_TEST); // z_buffer
    glEnable (GL_MULTISAMPLE);

    // Prepare a complete shader program...
    // We can't call setShader because of initialization issues
    if (m_program) {
        m_program->release();
        delete(m_program);
    }
	QString shaderPath = workingDirectory + "../shaders/";
    m_program = prepareShaderProgram(shaderPath + "1_simple.vert", shaderPath + "1_simple.frag");
    if (ground_program) {
        ground_program->release();
        delete(ground_program);
    }
    ground_program = prepareShaderProgram(shaderPath + "3_textured.vert", shaderPath + "3_textured.frag");
    if (ground2_program) {
        ground2_program->release();
        delete(ground2_program);
    }
    ground2_program = prepareShaderProgram(shaderPath + "3_textured.vert", shaderPath + "3_textured.frag");
    if(joints_program) {
      joints_program -> release();
      delete(joints_program);
    }
    joints_program = prepareShaderProgram(shaderPath + "animation.vert", shaderPath + "animation.frag");
    if (shadowMapGenerationProgram) {
        shadowMapGenerationProgram->release();
        delete(shadowMapGenerationProgram);
    }
    shadowMapGenerationProgram = prepareShaderProgram(shaderPath + "h_shadowMapGeneration.vert", shaderPath + "h_shadowMapGeneration.frag");

    // loading texture:
    loadTexturesForShaders();
    m_vao.create();
    m_vao.bind();
    m_vertexBuffer.create();
    m_indexBuffer.create();
    m_colorBuffer.create();
    m_normalBuffer.create();
    m_texcoordBuffer.create();
    if (width() > height()) m_screenSize = width(); else m_screenSize = height();
    initPermTexture(); // create Perlin noise texture
    m_vao.release();

    ground_vao.create();
    ground_vao.bind();
    ground_vertexBuffer.create();
    ground_indexBuffer.create();
    ground_colorBuffer.create();
    ground_normalBuffer.create();
    ground_texcoordBuffer.create();
    ground_vao.release();

    // DEUXIEME
    ground2_vao.create();
    ground2_vao.bind();
    ground2_vertexBuffer.create();
    ground2_indexBuffer.create();
    ground2_colorBuffer.create();
    ground2_normalBuffer.create();
    ground2_texcoordBuffer.create();
    ground2_vao.release();


    // Partie des joints
    joints_vao.create();
    joints_vao.bind();
    joints_vertexBuffer.create();
    joints_indexBuffer.create();
    joints_normalBuffer.create();
    joints_colorBuffer.create();
    joints_texcoordBuffer.create();
    joints_vao.release();
    openScene();
}

void glShaderWindow::resizeEvent(QResizeEvent* event)
{
   OpenGLWindow::resizeEvent(event);
   resize(event->size().width(), event->size().height());
}

void glShaderWindow::resize(int x, int y)
{
    OpenGLWindow::resize(x,y);
    // for GPGPU: since computeResult is the size of the window, it must be recreated at each window resize.
    if (computeResult) {
        computeResult->release();
        computeResult->destroy();
        delete computeResult;
        computeResult = 0;
    }
    if (hasComputeShaders) {
        m_program->bind();
        computeResult = new QOpenGLTexture(QImage(pixels, x, y, QImage::Format_RGBA8888));
        if (computeResult) {
            computeResult->setWrapMode(QOpenGLTexture::MirroredRepeat);
            computeResult->setMinificationFilter(QOpenGLTexture::Nearest);
            computeResult->setMagnificationFilter(QOpenGLTexture::Nearest);
            computeResult->bind(2);
        }
        m_program->release();
    }
    // Normal case: we rework the perspective projection for the new viewport, based on smaller size
    if (x > y) m_screenSize = x; else m_screenSize = y;
    if (m_program && modelMesh) {
        QMatrix4x4 persp;
        float radius = modelMesh->bsphere.r;
        m_program->bind();
        m_perspective.setToIdentity();
        if (x > y)
            m_perspective.perspective(60, (float)x/y, 0.1 * radius, 20 * radius);
        else {
            m_perspective.perspective((240.0/M_PI) * atan((float)y/x), (float)x/y, 0.1 * radius, 20 * radius);
        }
        renderNow();
    }
}

QOpenGLShaderProgram* glShaderWindow::prepareShaderProgram(const QString& vertexShaderPath,
                               const QString& fragmentShaderPath)
{
    QOpenGLShaderProgram* program = new QOpenGLShaderProgram(this);
    if (!program) qWarning() << "Failed to allocate the shader";
    bool result = program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderPath);
    if ( !result )
        qWarning() << program->log();
    result = program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderPath);
    if ( !result )
        qWarning() << program->log();
    result = program->link();
    if ( !result )
        qWarning() << program->log();
    return program;
}

QOpenGLShaderProgram* glShaderWindow::prepareComputeProgram(const QString& computeShaderPath)
{
    QOpenGLShaderProgram* program = new QOpenGLShaderProgram(this);
    if (!program) qWarning() << "Failed to allocate the shader";
    bool result = program->addShaderFromSourceFile(QOpenGLShader::Compute, computeShaderPath);
    if ( !result )
        qWarning() << program->log();
    result = program->link();
    if ( !result )
        qWarning() << program->log();
    else hasComputeShaders = true;
    return program;
}

void glShaderWindow::setWorkingDirectory(QString& myPath, QString& myName, QString& texture, QString& envMap)
{
    workingDirectory = myPath;
    modelName = myPath + myName;
    textureName = myPath + "../textures/" + texture;
    envMapName = myPath + "../textures/" + envMap;
}

void glShaderWindow::mouseToTrackball(QVector2D &mousePosition, QVector3D &spacePosition)
{
    const float tbRadius = 0.8f;
    float r2 = mousePosition.x() * mousePosition.x() + mousePosition.y() * mousePosition.y();
    const float t2 = tbRadius * tbRadius / 2.0;
    spacePosition = QVector3D(mousePosition.x(), mousePosition.y(), 0.0);
    if (r2 < t2) {
        spacePosition.setZ(sqrt(2.0 * t2 - r2));
    } else {
        spacePosition.setZ(t2 / sqrt(r2));
    }
}

// virtual trackball implementation
void glShaderWindow::mousePressEvent(QMouseEvent *e)
{
    lastMousePosition = (2.0/m_screenSize) * (QVector2D(e->localPos()) - QVector2D(0.5 * width(), 0.5*height()));
    mouseToTrackball(lastMousePosition, lastTBPosition);
    if (isFullrt){
        changeShader = true;
        setShader("2_phong");
    }
    mouseButton = e->button();
}

void glShaderWindow::wheelEvent(QWheelEvent * ev)
{
    if (isFullrt){
        changeShader = true;
        setShader("2_phong");
        usedWheel=true;
    }
    int matrixMoving = 0;
    if (ev->modifiers() & Qt::ShiftModifier) matrixMoving = 1;
    else if (ev->modifiers() & Qt::AltModifier) matrixMoving = 2;

    QPoint numDegrees = ev->angleDelta() /(float) (8 * 3.0);
    if (matrixMoving == 0) {
        QMatrix4x4 t;
        t.translate(0.0, 0.0, numDegrees.y() * modelMesh->bsphere.r / 100.0);
        m_matrix[matrixMoving] = t * m_matrix[matrixMoving];
    } else  if (matrixMoving == 1) {
        lightDistance -= 0.1 * numDegrees.y();
    } else  if (matrixMoving == 2) {
        groundDistance += 0.1 * numDegrees.y();
    }
    renderNow();
}

void glShaderWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (changeShader && usedWheel){
        changeShader = false;
        setShader("gpgpu_fullrt");
        usedWheel = false;
    }
    if (mouseButton == Qt::NoButton) return;
    QVector2D mousePosition = (2.0/m_screenSize) * (QVector2D(e->localPos()) - QVector2D(0.5 * width(), 0.5*height()));
    QVector3D currTBPosition;
    mouseToTrackball(mousePosition, currTBPosition);
    int matrixMoving = 0;
    if (e->modifiers() & Qt::ShiftModifier) matrixMoving = 1;
    else if (e->modifiers() & Qt::AltModifier) matrixMoving = 2;

    switch (mouseButton) {
    case Qt::LeftButton: {
        QVector3D rotAxis = QVector3D::crossProduct(lastTBPosition, -currTBPosition);
        float rotAngle = (180.0/M_PI) * rotAxis.length() /(lastTBPosition.length() * currTBPosition.length()) ;
        rotAxis.normalize();
        QQuaternion rotation = QQuaternion::fromAxisAndAngle(rotAxis, rotAngle);
        m_matrix[matrixMoving].translate(m_center);
        m_matrix[matrixMoving].rotate(rotation);
        m_matrix[matrixMoving].translate(- m_center);
        break;
    }
    case Qt::RightButton: {
        QVector2D diff = 0.2 * m_screenSize * (mousePosition - lastMousePosition);
        if (matrixMoving == 0) {
            QMatrix4x4 t;
            t.translate(diff.x() * modelMesh->bsphere.r / 100.0, -diff.y() * modelMesh->bsphere.r / 100.0, 0.0);
            m_matrix[matrixMoving] = t * m_matrix[matrixMoving];
        } else if (matrixMoving == 1) {
            lightDistance += 0.1 * diff.y();
        } else  if (matrixMoving == 2) {
            groundDistance += 0.1 * diff.y();
        }
        break;
    }
	default: break;
    }
    lastTBPosition = currTBPosition;
    lastMousePosition = mousePosition;
    renderNow();
}

void glShaderWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (changeShader){
        setShader("gpgpu_fullrt");
        changeShader = false;
        animating = false;

    }
    mouseButton = Qt::NoButton;
}

void glShaderWindow::keyPressEvent(QKeyEvent* e)
{
    int key = e->key();
    switch (key)
    {
        case Qt::Key_Space:
            animating = true;
            root->animate(frame);
            frame++;
            toggleAnimating();
            break;
        default:
            break;
    }
}

void glShaderWindow::timerEvent(QTimerEvent *e)
{

}

static int nextPower2(int x) {
    // returns the first power of 2 above the argument
    // i.e. for 12 returns 4 (because 2^4 = 16 > 12)
    if (x == 0) return 1;

    x -= 1;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x += 1;
    return x;
}



void glShaderWindow::render()
{
    QVector3D lightPosition = m_matrix[1] * (m_center + lightDistance * modelMesh->bsphere.r * QVector3D(0.5, 0.5, 1));

    QMatrix4x4 lightCoordMatrix;
    QMatrix4x4 lightPerspective;
    QMatrix4x4 mat_inverse = m_matrix[0];
    QMatrix4x4 persp_inverse = m_perspective;

    if (isGPGPU || hasComputeShaders) {
        bool invertible;
        mat_inverse = mat_inverse.inverted(&invertible);
        persp_inverse = persp_inverse.inverted(&invertible);
    }
    if (hasComputeShaders) {
        // We bind the texture generated to texture unit 2 (0 is for the texture, 1 for the env map)
#ifndef __APPLE__
               glActiveTexture(GL_TEXTURE2);
        compute_program->bind();
		computeResult->bind(2);
        // Send parameters to compute program:
        compute_program->setUniformValue("bbmin", m_bbmin);
        compute_program->setUniformValue("bbmax", m_bbmax);
        compute_program->setUniformValue("center", m_center);
        compute_program->setUniformValue("radius", modelMesh->bsphere.r);
        compute_program->setUniformValue("groundDistance", groundDistance * modelMesh->bsphere.r - m_center[1]);
        compute_program->setUniformValue("mat_inverse", mat_inverse);
        compute_program->setUniformValue("persp_inverse", persp_inverse);
        compute_program->setUniformValue("lightPosition", lightPosition);
        compute_program->setUniformValue("lightIntensity", 1.0f);
        compute_program->setUniformValue("blinnPhong", blinnPhong);
        compute_program->setUniformValue("transparent", transparent);
        compute_program->setUniformValue("lightning", lightning);
        compute_program->setUniformValue("bubble", bubble);
        compute_program->setUniformValue("animating",animating);
        compute_program->setUniformValue("animation_time",animation_time);
        compute_program->setUniformValue("lightIntensity", lightIntensity);
        compute_program->setUniformValue("refractions", refractions);
        compute_program->setUniformValue("innerRadius", innerRadius);
        compute_program->setUniformValue("shininess", shininess);
        compute_program->setUniformValue("eta", eta);
        // compute_program->setUniformValue("eta.y", eta.y);
        compute_program->setUniformValue("framebuffer", 2);
        compute_program->setUniformValue("colorTexture", 0);
		glBindImageTexture(2, computeResult->textureId(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);
        int worksize_x = nextPower2(width());
        int worksize_y = nextPower2(height());
        glDispatchCompute(worksize_x / compute_groupsize_x, worksize_y / compute_groupsize_y, 1);
        glBindImageTexture(2, 0, 0, false, 0, GL_READ_ONLY, GL_RGBA32F);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        compute_program->release();
#endif
	} else if ((ground_program->uniformLocation("shadowMap") != -1) ||(ground2_program->uniformLocation("shadowMap") != -1) || (m_program->uniformLocation("shadowMap") != -1) || (joints_program->uniformLocation("shadowMap") != -1) ){
		glActiveTexture(GL_TEXTURE2);
        // The program uses a shadow map, let's compute it.
		glBindFramebuffer(GL_FRAMEBUFFER, shadowMap_fboId);
        glViewport(0, 0, shadowMapDimension, shadowMapDimension);
        // Render into shadow Map
        shadowMapGenerationProgram->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
        glDisable(GL_CULL_FACE); // mainly because some models intersect with the gronud
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // set up camera position in light source:
        // TODO_shadowMapping: you must initialize these two matrices.
        lightCoordMatrix.setToIdentity();
        lightPerspective.setToIdentity();

        shadowMapGenerationProgram->setUniformValue("matrix", lightCoordMatrix);
        shadowMapGenerationProgram->setUniformValue("perspective", lightPerspective);
        // Draw the entire scene:
        m_vao.bind();
        glDrawElements(GL_TRIANGLES, 3 * m_numFaces, GL_UNSIGNED_INT, 0);
        m_vao.release();
        ground_vao.bind();
        glDrawElements(GL_TRIANGLES, g_numIndices, GL_UNSIGNED_INT, 0);
        ground_vao.release();
        ground2_vao.bind();
        glDrawElements(GL_TRIANGLES, g2_numIndices, GL_UNSIGNED_INT, 0);
        ground2_vao.release();
        joints_vao.bind();
        glDrawElements(GL_LINES,j_numIndices,GL_UNSIGNED_INT,0);
        joints_vao.release();
        glFinish();
        // done. Back to normal drawing.
        shadowMapGenerationProgram->release();
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // unbind
        glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
        glEnable(GL_CULL_FACE);
        glCullFace (GL_BACK); // cull back face
		glBindTexture(GL_TEXTURE_2D, shadowMap_textureId);
    }
    m_program->bind();
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (isGPGPU) {
        m_program->setUniformValue("computeResult", 2);
        m_program->setUniformValue("center", m_center);
        m_program->setUniformValue("mat_inverse", mat_inverse);
        m_program->setUniformValue("persp_inverse", persp_inverse);
    } else {
        m_program->setUniformValue("matrix", m_matrix[0]);
        m_program->setUniformValue("perspective", m_perspective);
        m_program->setUniformValue("lightMatrix", m_matrix[1]);
        m_program->setUniformValue("normalMatrix", m_matrix[0].normalMatrix());
    }
    m_program->setUniformValue("lightPosition", lightPosition);
    m_program->setUniformValue("lightIntensity", 1.0f);
    m_program->setUniformValue("blinnPhong", blinnPhong);
    m_program->setUniformValue("transparent", transparent);
    m_program->setUniformValue("bubble", bubble);
    m_program->setUniformValue("lightning", lightning);
    m_program->setUniformValue("animating", animating);
    m_program->setUniformValue("animation_time", animation_time);
    m_program->setUniformValue("lightIntensity", lightIntensity);
    m_program->setUniformValue("refractions", refractions);
    m_program->setUniformValue("innerRadius", innerRadius);
    m_program->setUniformValue("shininess", shininess);
    m_program->setUniformValue("eta", eta);
    // m_program->setUniformValue("eta.y", eta.y);
    m_program->setUniformValue("radius", modelMesh->bsphere.r);
	if (m_program->uniformLocation("colorTexture") != -1) m_program->setUniformValue("colorTexture", 0);
    if (m_program->uniformLocation("envMap") != -1)  m_program->setUniformValue("envMap", 1);
	else if (m_program->uniformLocation("permTexture") != -1)  m_program->setUniformValue("permTexture", 1);
    // Shadow Mapping
    if (m_program->uniformLocation("shadowMap") != -1) {
        m_program->setUniformValue("shadowMap", 2);
        // TODO_shadowMapping: send the right transform here
    }
    if (isGPGPU){
      m_vao.bind();
      glDrawElements(GL_TRIANGLES, 3 * m_numFaces, GL_UNSIGNED_INT, 0);
      m_vao.release();
      m_program->release();
    }


    if (!isGPGPU) {
        // also draw the ground, with a different shader program
        ground_program->bind();
        ground_program->setUniformValue("lightPosition", lightPosition);
        ground_program->setUniformValue("matrix", m_matrix[0]);
        ground_program->setUniformValue("lightMatrix", m_matrix[1]);
        ground_program->setUniformValue("perspective", m_perspective);
        ground_program->setUniformValue("normalMatrix", m_matrix[0].normalMatrix());
        ground_program->setUniformValue("lightIntensity", 1.0f);
        ground_program->setUniformValue("blinnPhong", blinnPhong);
        ground_program->setUniformValue("transparent", transparent);
        ground_program->setUniformValue("bubble", bubble);
        ground_program->setUniformValue("lightning", lightning);
        ground_program->setUniformValue("animating", animating);
        ground_program->setUniformValue("animation_time", animation_time);
        ground_program->setUniformValue("lightIntensity", lightIntensity);
        ground_program->setUniformValue("refractions", refractions);
        ground_program->setUniformValue("innerRadius", innerRadius);
        ground_program->setUniformValue("shininess", shininess);
        ground_program->setUniformValue("eta", eta);
        // ground_program->setUniformValue("eta.y", eta.y);
        ground_program->setUniformValue("radius", modelMesh->bsphere.r);
		if (ground_program->uniformLocation("colorTexture") != -1) ground_program->setUniformValue("colorTexture", 0);
        if (ground_program->uniformLocation("shadowMap") != -1) {
            ground_program->setUniformValue("shadowMap", 2);
            // TODO_shadowMapping: send the right transform here
        }
        ground_vao.bind();
        glDrawElements(GL_LINES, g_numIndices, GL_UNSIGNED_INT, 0);
        ground_vao.release();
        ground_program->release();
    }

    if (!isGPGPU) {
        // also draw the ground2, with a different shader program
        ground2_program->bind();
        ground2_program->setUniformValue("lightPosition", lightPosition);
        ground2_program->setUniformValue("matrix", m_matrix[0]);
        ground2_program->setUniformValue("lightMatrix", m_matrix[1]);
        ground2_program->setUniformValue("perspective", m_perspective);
        ground2_program->setUniformValue("normalMatrix", m_matrix[0].normalMatrix());
        ground2_program->setUniformValue("lightIntensity", 1.0f);
        ground2_program->setUniformValue("blinnPhong", blinnPhong);
        ground2_program->setUniformValue("transparent", transparent);
        ground2_program->setUniformValue("bubble", bubble);
        ground2_program->setUniformValue("lightning", lightning);
        ground2_program->setUniformValue("animating", animating);
        ground2_program->setUniformValue("animation_time", animation_time);
        ground2_program->setUniformValue("lightIntensity", lightIntensity);
        ground2_program->setUniformValue("refractions", refractions);
        ground2_program->setUniformValue("innerRadius", innerRadius);
        ground2_program->setUniformValue("shininess", shininess);
        ground2_program->setUniformValue("eta", eta);
        ground2_program->setUniformValue("radius", modelMesh->bsphere.r);
		if (ground2_program->uniformLocation("colorTexture") != -1) ground2_program->setUniformValue("colorTexture", 0);
        if (ground2_program->uniformLocation("shadowMap") != -1) {
            ground2_program->setUniformValue("shadowMap", 2);
        }
        ground2_vao.bind();
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR){
          std::cout << "GL error: " << err << std::endl;
        }
        int nb_arrete = ((g2_numIndices)*2);
        glDrawElements(GL_LINES, nb_arrete, GL_UNSIGNED_INT, 0);
        while ((err = glGetError()) != GL_NO_ERROR){
          std::cout << "GL error: " << err << std::endl;
        }

        if(animating){
          g2_numIndices = 0;

          updateJoints(root);
          bindSceneToProgram();
          animating = false;
        }
        /*
        for(int i = 0 ; i < g2_numPoints; i++){
          std::cout << " Vertices : " << g2_vertices[i] << std::endl;
        }*/

        ground2_vao.release();
        ground2_program->release();
    }
}
