#include "joint.h"
#include <QtGui/QMatrix4x4>
#include <sstream>
#include <fstream>

using namespace std;

Joint* Joint::createFromFile(std::string fileName) {
	Joint* root = new Joint();
	cout << "Loading from " << fileName << endl;

	ifstream inputfile(fileName.data());
	if(inputfile.good()) {
		while(!inputfile.eof()) {

			std::string buf;
			inputfile >> buf; // read HIERARCHY
			if (strcmp(buf.data(),"HIERARCHY") == 0){
				inputfile >> buf; // read ROOT
				if (strcmp(buf.data(),"ROOT") != 0){
					return NULL;
				}
				root = read_joint(inputfile);

			} else if (strcmp(buf.data(),"MOTION") == 0){
				// Remplir les motions en parcourant l'arbre de root
				inputfile >> buf;
				if (strcmp(buf.data(), "Frames:") == 0){
					inputfile >> buf;
					int nbr_frames = std::stoi(buf);
					inputfile >> buf;
					inputfile >> buf;
					inputfile >> buf; // frame time
					inputfile >> buf; // On arrive sur le début des valeurs
					int nb_channels = root->nbChannels();
					std::cout << "nbrChannels = " << nb_channels << std::endl;
					std::vector<std::vector<double>> values = std::vector<std::vector<double>>(nb_channels, std::vector<double>(nbr_frames));
					// On boucle sur les lignes de frames
					for (int j = 0; j < nbr_frames; j++){
						std::getline(inputfile,buf) >> std::ws;
						stringstream ss(buf);
						int i = 0;
						while(!ss.eof()){
							ss >> buf >> std::ws;
							values[i][j] = std::stod(buf);
							i++;
						}
					}
					// Parcours de l'arbre en profondeur pour remplir les AnimCurve
					int previous_nb_c = 0;
					root->fill_AnimCurves(previous_nb_c, values);
				}
			}
		}
		cout << "file loaded" << endl;
		inputfile.close();
	} else {
		std::cerr << "Failed to load the file " << fileName.data() << std::endl;
		fflush(stdout);
	}

	return root;
}

void Joint::animate(int iframe)
{
	// Update dofs :
	_curTx = 0; _curTy = 0; _curTz = 0;
	_curRx = 0; _curRy = 0; _curRz = 0;
	for (unsigned int idof = 0 ; idof < _dofs.size() ; idof++) {
		if(!_dofs[idof].name.compare("Xposition")) _curTx = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Yposition")) _curTy = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Zposition")) _curTz = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Zrotation")) _curRz = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Yrotation")) _curRy = _dofs[idof]._values[iframe];
		if(!_dofs[idof].name.compare("Xrotation")) _curRx = _dofs[idof]._values[iframe];
	}
	// Animate children :
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		_children[ichild]->animate(iframe);
	}
}


void Joint::nbDofs() {
	if (_dofs.empty()) return;

	double tol = 1e-4;

	int nbDofsR = -1;

	// TODO :
	cout << _name << " : " << nbDofsR << " degree(s) of freedom in rotation\n";

	// Propagate to children :
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		_children[ichild]->nbDofs();
	}

}

int Joint::nbChannels(){
	int nbr = _dofs.size();
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		 nbr += _children[ichild]->nbChannels();
	}
	return nbr;
}

Joint* Joint::read_joint(std::ifstream &inputfile){

	Joint* cur = new Joint();
	string buf;

	inputfile >> buf; // read joint name
	cur->_name = buf;
	inputfile >> buf; // read {
	inputfile >> buf; // read OFFSET
	if (strcmp(buf.data(),"OFFSET") != 0){
		return NULL;
	}
	inputfile >> buf; // read offX
	cur->_offX = std::stod(buf);
	inputfile >> buf; // read offY
	cur->_offY = std::stod(buf);
	inputfile >> buf; // read offZ
	cur->_offZ = std::stod(buf);
	inputfile >> buf; // read CHANNELS
	if (strcmp(buf.data(),"CHANNELS") != 0){
		return NULL;
	}
	inputfile >> buf; // read nb CHANNELS
	int nbr_channels = std::stoi(buf);
	cur->_dofs.reserve(nbr_channels);
	for (int i = 0; i < nbr_channels; i++){
		inputfile >> buf; // read i-th channels
		AnimCurve* anim = new AnimCurve();
		anim->name = buf;
		anim->_values.clear();
		cur->_dofs.push_back(*anim);
	}
	inputfile >> buf; // read JOINT ou END ou }

	while(strcmp(buf.data(),"}") != 0){
		if (strcmp(buf.data(),"JOINT") == 0){
			cur->_children.push_back(read_joint(inputfile));
		} else if (strcmp(buf.data(),"End") == 0){
			cur->_children.push_back(read_end(inputfile));
		} else {
			return NULL;
		}
		inputfile >> buf; // read JOINT ou END ou }
	}

	return cur;

}

Joint* Joint::read_end(std::ifstream &inputfile){

	Joint* cur = new Joint();
	string buf;

	inputfile >> buf; // read joint name
	cur->_name = buf;
	inputfile >> buf; // read {
	inputfile >> buf; // read OFFSET
	if (strcmp(buf.data(),"OFFSET") != 0){
		return NULL;
	}
	inputfile >> buf; // read offX
	cur->_offX = std::stod(buf);
	inputfile >> buf; // read offY
	cur->_offY = std::stod(buf);
	inputfile >> buf; // read offZ
	cur->_offZ = std::stod(buf);
	inputfile >> buf; // read }
	if (strcmp(buf.data(),"}") != 0){
		return NULL;
	}

	return cur;

}

void Joint::fill_AnimCurves(int& previous_nb_c, std::vector<std::vector<double>> values){
	// remplir le joint actuel
	int nbr_channels = _dofs.size();
	for (int i = 0; i < nbr_channels; i++){
		_dofs[i]._values = values[previous_nb_c+i];
	}
	// filling _rorder
	if (_dofs.size() > 0){
		const char* c0 = (_dofs[0].name).c_str();
		const char* c1 = (_dofs[1].name).c_str();
		if (strcmp(c0,"Xrotation") == 0){
			if (strcmp(c1,"Yrotation") == 0){
				_rorder = 0;
			} else {
				_rorder = 3;
			}
		} else if (strcmp(c0,"Yrotation") == 0){
			if (strcmp(c1,"Xrotation") == 0){
				_rorder = 4;
			} else {
				_rorder = 1;
			}
		} else {
			if (strcmp(c1,"Xrotation") == 0){
				_rorder = 2;
			} else {
				_rorder = 5;
			}
		}
	}
	previous_nb_c += nbr_channels;
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		//if(_children[ichild]->_dofs[0]._values.empty()){
		_children[ichild]->fill_AnimCurves(previous_nb_c, values);
		//}
	}
}
