#include "joint.h"
#include <QtGui/QMatrix4x4>
#include <sstream>

using namespace std;

Joint* Joint::createFromFile(std::string fileName) {
	Joint* root = NULL;
	cout << "Loading from " << fileName << endl;

	ifstream inputfile(fileName.data());
	if(inputfile.good()) {
		while(!inputfile.eof()) {
			string buf;
			inputfile >> buf;
			if (strcmp(buf.data(),"HIERARCHY") == 0){
				inputfile >> buf;
				// On remplit le Joint root
				while(strcmp(buf.data(),"JOINT") != 0){
					if (strcmp(buf.data(),"ROOT") == 0){
						inputfile >> buf;
						root->_name = buf;
						inputfile >> buf;
					} else if (strcmp(buf.data(),"OFFSET") == 0){
							inputfile >> buf;
							root->_offX = std::stod(buf);
							inputfile >> buf;
							root->_offY = std::stod(buf);
							inputfile >> buf;
							root->_offZ = std::stod(buf);
							inputfile >> buf;
					} else if (strcmp(buf.data(),"CHANNELS") == 0){
						inputfile >> buf;
						int nbr_channels = std::stoi(buf);
						root->_dofs.reserve(nbr_channels);
						for (int i = 0; i < nbr_channels; i++){
							inputfile >> buf;
							AnimCurve* anim = new AnimCurve();
							anim->name = buf;
							anim->_values.clear();
							root->_dofs.push_back(*anim);
						}
						inputfile >> buf;
					} else {
						inputfile >> buf;
					}
				}
				// Normalement, on arrive à un joint fils
				if (strcmp(buf.data(),"JOINT") == 0){
					Joint* children = read_joint(inputfile,root);
				}
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
					std::vector<std::vector<double>> values = std::vector<std::vector<double>>(nbr_frames);
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
		inputfile.close();
	} else {
		std::cerr << "Failed to load the file " << fileName.data() << std::endl;
		fflush(stdout);
	}

	cout << "file loaded" << endl;

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

Joint* Joint::read_joint(std::ifstream &inputfile, Joint* parent){

	Joint* cur = new Joint();
	string buf;
	inputfile >> buf;

	// On a trouve un joint qui defini la fin de la branche
	if (strcmp(buf.data(),"End") == 0){
		// Nom du joint
		inputfile >> buf;
		cur->_name = buf;
		inputfile >> buf;
		// Tant qu'on n'est pas a la fin du joint
		while (strcmp(buf.data(),"}") != 0){
			if (strcmp(buf.data(),"OFFSET") == 0){
				inputfile >> buf;
				cur->_offX = std::stod(buf);
				inputfile >> buf;
				cur->_offY = std::stod(buf);
				inputfile >> buf;
				cur->_offZ = std::stod(buf);
				inputfile >> buf;
			} else {
				inputfile >> buf;
			}
		}
		// On a atteint la fin du joint
		if(parent != NULL) {
			parent->_children.push_back(cur);
		}
		// On renvoit le parent du joint courant
		return parent;

	// On a trouve un joint (pas la fin)
	} else if (strcmp(buf.data(),"JOINT") == 0){
		// Nom du joint
		inputfile >> buf;
		cur->_name = buf;
		inputfile >> buf;
		// Fin du joint = autre joint ou end
		// Tant qu'on n'est pas a la fin du joint
		while (strcmp(buf.data(),"JOINT") != 0 || strcmp(buf.data(),"End") != 0){
			if (strcmp(buf.data(),"OFFSET") == 0){
				inputfile >> buf;
				cur->_offX = std::stod(buf);
				inputfile >> buf;
				cur->_offY = std::stod(buf);
				inputfile >> buf;
				cur->_offZ = std::stod(buf);
				inputfile >> buf;
			} else if (strcmp(buf.data(),"CHANNELS") == 0){
				inputfile >> buf;
				int nbr_channels = std::stoi(buf);
				cur->_dofs.reserve(nbr_channels);
				for (int i = 0; i < nbr_channels; i++){
					inputfile >> buf;
					AnimCurve* anim = new AnimCurve();
					anim->name = buf;
					anim->_values.clear();
					cur->_dofs.push_back(*anim);
				}
				inputfile >> buf;
			} else {
				inputfile >> buf;
			}
		}
		if(parent != NULL) {
			parent->_children.push_back(cur);
		}
		// On a retrouvé un joint fils
		return read_joint(inputfile,cur);
	// Si on trouve une accolade fermante
	} else if (strcmp(buf.data(), "}") == 0){
		return parent;
	}
}

void Joint::fill_AnimCurves(int& previous_nb_c, std::vector<std::vector<double>> values){
	// remplir le joint actuel
	int nbr_channels = _dofs.size();
	for (int i = 0; i < nbr_channels; i++){
		_dofs[i]._values = values[previous_nb_c+i];
	}
	previous_nb_c += nbr_channels;
	for (unsigned int ichild = 0 ; ichild < _children.size() ; ichild++) {
		if(_children[ichild]->_dofs[0]._values.empty()){
			_children[ichild]->fill_AnimCurves(previous_nb_c, values);
		}
	}
}
