////////////////////////////////////////////////////////////////
//devoir3.cpp
//Créé par: Sara Séguin
//Date: 24 octobre 2017
//Modifié par:Sara Séguin
//Date de modification: 15 novembre 2021
//Description:
//CE FICHIER N'EST PAS EXHAUSTIF, IL DONNE SIMPLEMENT DES EXEMPLES ET UNE CERTAINE STRUCTURE.
////////////////////////////////////////////////////////////////
	//soft string
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <bitset>
#include <fstream>
#include <string>

#define PAGE_t 256 //Taille d'une page (256 bytes)
#define FRAME_t 256

using PageTable = int[256][2];
struct TLBEntry {
	bool defined=false;
  	int frameIndex;
  	int pageIndex;
};

////////////////////////////////////////////////////////////////
//Cette fonction créé un masque afin de lire les bits nécessaires. NE PAS MODIFIER ET UTILISER TEL QUEL DANS LE MAIN
//Créé par: Sara Séguin
//Modifié par:
//Description:
////////////////////////////////////////////////////////////////
unsigned createMask(unsigned a, unsigned b)
{

	unsigned r = 0;
	for(unsigned i=a;i<=b;i++)
	{
		r |= 1 << i;
	}
	return r;
}


void history_more(std::string Line)
{
	//ouverture du fichier historique
    std::fstream historique("Historique.txt", std::ios::out | std::ios::app);
	//écriture de la ligne et saut de ligne
    historique << Line.c_str() << "\n";
	// fermeture du fichier
    historique.close();
}

////////////////////////////////////////////////////////////////
//Cette fonction retourne la valeur du byte signé
////////////////////////////////////////////////////////////////
int fct_SignedByte(int page, int offset)
{
    char SignedByte;
    std::ifstream nomduFichier ("simuleDisque.bin", std::ifstream::binary);
    nomduFichier.exceptions( std::ifstream::badbit);
    //ouverture du  le fichier simule disque binaire
    try {
        if(nomduFichier){
            unsigned int LENGTH = 1; //Le byte signé a une longueur de 1 byte
            nomduFichier.seekg(page*PAGE_t+offset); //Trouver l'endroit correspondant au byte signé dans le fichier
            nomduFichier.read(&SignedByte,LENGTH); //Lire cet emplacement
        }
    }
    catch (const std::ifstream::failure& e) {
        SignedByte = -1;
        std::cout << "Exception opening/reading file";
      }

    //Fermer le fichier
    nomduFichier.close();


    //Retourner la valeur du byte signé
    return SignedByte;
}

int getNbrFrame(int pageNumber, int * currentFrame, int * memPhysique,int * NbrPageFault, int * NbrTLBSuccess,  PageTable * tablePage, std::vector<TLBEntry> * TLB)
{
		//verification si dans le TLB il y a la table
		for(int i=0;i<TLB->size();i++){


			TLBEntry tlbEntry=(*TLB)[i];


			if(tlbEntry.pageIndex==pageNumber && tlbEntry.defined==true){

				//comme utiliser déplacement vers la fin
				TLB->push_back((*TLB)[i]);
				TLB->erase(TLB->begin()+i,TLB->begin()+i+1);

				//incrémenatation du nombre de TLB success
				++*NbrTLBSuccess;


				return (TLB->back()).frameIndex;
				
			}	
		}
		
		int NbrFrame=*currentFrame;

		//Si page deja chargée stockage du frame 
		bool alreadyLoaded=false;
		if((*tablePage)[pageNumber][1] == 1)
		{
			NbrFrame=(*tablePage)[pageNumber][0];
			return NbrFrame;
		}
		
		//remplace la plus ancienne frame par la nouvelle dans le TLB
		TLB->erase(TLB->begin(),TLB->begin()+1);

		//création d'un TLB Entry
		TLBEntry Entry = TLBEntry();
		Entry.defined=true;
		Entry.frameIndex=NbrFrame;
		Entry.pageIndex=pageNumber;
		TLB->push_back(Entry);


		//incrémentation du nbr de page fault;
		++*NbrPageFault;

		// Chargement de la page dans la frame actuelle

		// On itère à travers chaqye byte de la page pour le charger dans la mémoire physique
		for (int byteIndex = 0; byteIndex < FRAME_t; byteIndex++)
		{
			memPhysique[NbrFrame * FRAME_t + byteIndex] = fct_SignedByte(pageNumber, byteIndex);
		}

		// Mise a jour de la table de frame 
		(*tablePage)[pageNumber][1] = 1;
		(*tablePage)[pageNumber][0] = *currentFrame;

		//incrémantation de currentFrame
		++*currentFrame;
		
		//renvoi NbrFrame
		return NbrFrame;
}

int GetValueMemory(int memPhysique[65536], int physicalAddress)
{
	//  masque pour les bits 0 à 7 (offset)
	unsigned MaskOffset = 0;
	MaskOffset = createMask(0, 7);

	// masque pour les bits 8 à 15 (page)
	unsigned MaskFrame = 0;
	MaskFrame = createMask(8, 15);

	// récuperation du offset et du Frame
	int Offset = physicalAddress & MaskOffset;
	int Frame = physicalAddress & MaskFrame;	
	Frame = Frame >> 8;

	return memPhysique[Frame * FRAME_t + Offset];
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
///////////////// PROGRAMME PRINCIPAL ////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

int main()
{
	std::remove("Historique.txt"); // suppresion si il y a un fichier historique qui existe.

	//Initialisation et déclarations
	int memPhysique[65536] = {0}; //Mémoire physique
	int adressePhysique[1000] = {0}; //Adresses Physiques
	int tablePage[256][2]={0}; //Table de page
	std::vector<int>adresseLogique; //Adresses Logiques
	int currentFrame = 0;
	std::vector<TLBEntry> TLB;
	for(int i=0;i<16;i++){
		TLB.push_back(TLBEntry());
	}

	int Nbr_pageFault=0;
	int Nbr_Success=0;
	
	
	//Lire le fichier d'adresses à traduire
	std::ifstream nomduFichier ("addresses.txt");

	std::string ligne;

	while(std::getline(nomduFichier,ligne)){ // lecture ligne par ligne
		int temp_int = std::stoi(ligne); // mise en format int
		adresseLogique.push_back(temp_int); // stockage dans le vecteur adresselogique

	//Stocker les nombres binaires dans un vecteur
	std::vector<int>bits_offset,bits_page; //Un vecteur pour les bits de page et un autre pour les bits d'offset
	
	//Crééer un masque pour lire juste les bits 0 à 7 (offset)
	unsigned r = 0;
	r = createMask(0,7);
	
	//Créer un masque pour lire juste les bits 8 à 15 (page)
	unsigned r2 = 0;
	r2 = createMask(8,15);
	
	//Boucler sur les 1000 adresse
	for(int i =0; i< adresseLogique.size(); i++)
	{
		unsigned result;	
		//EXEMPLE DE SYNTAXE POUR UTILISER LE MASQUE
		result = r & adresseLogique[i];
		std::string binary = std::bitset<8>(result).to_string(); //to binary
    	std::cout<<"offset: "<<binary<<"\n";

		unsigned result2;	
		//EXEMPLE DE SYNTAXE POUR UTILISER LE MASQUE
		result2 = r2 & adresseLogique[i];
		result2 = (result2 >> 8);
		std::string binary2 = std::bitset<8>(result2).to_string(); //to binary
    	std::cout<<"page: "<<binary2<<"\n";
		
		//Vecteurs de page et d'offset
		bits_offset.push_back(result);
		bits_page.push_back(result2);

		//création de l'adresse physique
		int frameNumber=getNbrFrame(result2,&currentFrame,&memPhysique[0],&Nbr_pageFault,&Nbr_Success,&tablePage,&TLB);
		int framePart= frameNumber << 8;
		int physicalAdresse=framePart+result;
		std::cout << "adresse physique :" << physicalAdresse << std::endl;

		//récupération de la valeur de la mémoire 
		int decimalValue=GetValueMemory(memPhysique,physicalAdresse);

		// mise en format string pour être ajouter a l'historique
		std::ostringstream Line;
		Line << "Virtual address: " << adresseLogique[i] << " Physical address: " << physicalAdresse << " Decimal value: " << decimalValue ;
		history_more(Line.str());
	}

	
	// statistique des page fault et success
	float Poucentage_page_Fault=(float(Nbr_pageFault)/float(1000))*100;
	float Pourcentage_TLB_Success=(float(Nbr_Success)/float(1000))*100;

	// mise en format string pour être ajouter a l'historique les statistiques
	std::ostringstream Line;
	Line << "Number of page faults: "<<Nbr_pageFault<<" nombre de TLB réussi : "<< Nbr_Success	<<"Poucentage Page fault: "<<Poucentage_page_Fault<<"% Poucentage TLB réussi: "<<Pourcentage_TLB_Success<<"% ";
	history_more(Line.str());

	return 0;

}

