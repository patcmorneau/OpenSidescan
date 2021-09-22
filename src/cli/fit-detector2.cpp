/*
* Copyright 2021 © Centre Interdisciplinaire de développement en Cartographie des Océans (CIDCO), Tous droits réservés
*/


#include <string>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include "../thirdParty/MBES-lib/src/utils/StringUtils.hpp"
#include "../OpenSidescan/sidescan/sidescanimager.h"
#include "../OpenSidescan/sidescan/sidescanfile.h"
#include "../OpenSidescan/sidescan/sidescanimage.h"
#include "../OpenSidescan/inventoryobject/inventoryobject.h"
#include "../OpenSidescan/detector/roidetector.h"

#include <Eigen/Dense>

#define POPULATION_SIZE 10
#define DECIMATION_SIZE 8

typedef struct{
    int channel;
    int x;
    int y;
} hit ;

typedef struct{
    int fastThreshold;
    int dbscanEpsilon;
    int dbscanMinPts;
    int mserDelta;
    int mserMinArea;
    int mserMaxArea;
    
    int fitness;
} genome;

void printUsage(){
    std::cerr << "fitDetector directory" << std::endl;
    exit(1);
}


void load_SSS_Files(std::vector<SidescanFile*> & files, std::string & directoryPath){
    DIR *dir = NULL;
    
    //TODO: read this lever arm from metadata or user input
    Eigen::Vector3d antenna2TowPoint(0.0,0.0,0.0);
    
    if(dir = opendir(directoryPath.c_str())){
        
        std::cerr << "Processing " << directoryPath << std::endl;
        
        dirent* file;
        
        std::stringstream filenameStream;
        
        while(file=readdir(dir)){
            
            if(StringUtils::ends_with(file->d_name,".xtf")){               
                filenameStream.str("");
                filenameStream << directoryPath << "/" << file->d_name;
                std::string fileName(filenameStream.str());
                
                std::cerr << "[+] Processing " << fileName << std::endl;
                
                std::cerr << "[-]     Reading image data from " << fileName << std::endl;                
                
                //Read image data
                SidescanImager imager;
                DatagramParser * parser = DatagramParserFactory::build(fileName,imager);
                parser->parse(fileName);

                SidescanFile * sidescanFile = imager.generate(fileName, antenna2TowPoint);

                files.push_back(sidescanFile);
            }
        }
        
        closedir(dir);
    }
    else{
        throw std::runtime_error("Can't open directory");
    }
}
void load_HITS_Files(std::vector<std::vector<hit*>*> & hits, std::string & directoryPath){
    DIR *dir = NULL;
    if(dir = opendir(directoryPath.c_str())){
        
        std::cerr << "Processing " << directoryPath << std::endl;
        
        dirent* file;
        
        std::stringstream filenameStream;
        
        while(file=readdir(dir)){
            if(StringUtils::ends_with(file->d_name,".hits")){ 
                filenameStream.str("");
                filenameStream << directoryPath << "/" << file->d_name;
                std::string hitsFilename = filenameStream.str();
                
                std::cerr << "[-]     Reading hits data from " << hitsFilename << std::endl;
                
                std::ifstream hitsFile(hitsFilename);
                
                if(hitsFile){
                    std::string line;
                    std::vector<hit*> * fileHits = new std::vector<hit*>();
                    
                    while(std::getline(hitsFile,line)){
                        hit * h = (hit*) malloc(sizeof(hit));
                        
                        if(sscanf(line.c_str(),"%d %d %d",&h->channel,&h->x,&h->y) == 3){
                            fileHits->push_back(h);
                        }
                        else{
                            std::cerr<<file->d_name<<"\n"; 
                            throw std::runtime_error("hits file not formatted properly"); //seems to be unstable
                        }
                    }
                    std::cerr << "[+]     " << fileHits->size() << " hits read" << std::endl;
                    hits.push_back(fileHits);
                    hitsFile.close();
                }
                
            }
            
        }
        
        closedir(dir);
    }
    else{
        throw std::runtime_error("Can't open directory");
    }
}

void randomize(genome * g){
        g->fastThreshold= rand() % 300 + 100;
        g->dbscanEpsilon= rand() % 100 + 10;
        g->dbscanMinPts = rand() % 10 + 10;
        g->mserDelta= rand() % 10 + 5;
        g->mserMinArea= rand() % 1000 + 10;
        g->mserMaxArea = 15000;    
}

void initGenomes(std::vector<genome*> & genomes){
    std::cerr << "[+] Initializing " << POPULATION_SIZE << " genomes" << std::endl;
    
    for(int i=0;i<POPULATION_SIZE;i++){
        genome * g = (genome*)malloc(sizeof(genome));
        
        randomize(g);
        
        genomes.push_back(g);
    }
}

bool insideHits(InventoryObject * obj,std::vector<hit*> & hits){
    for(auto i=hits.begin();i!=hits.end();i++){
        if(
                (*i)->channel == obj->getImage().getChannelNumber() &&
                ((*i)->x >= obj->getX()) && ((*i)->x <= (obj->getX() + obj->getPixelWidth())) &&
                ((*i)->y >= obj->getY()) && ((*i)->y <= (obj->getY() + obj->getPixelHeight()))
        ){
            return true;
        }
    }
    
    return false;
}

bool insideDetections(hit * h, std::vector<InventoryObject*> & detections){
    for(auto i=detections.begin();i!=detections.end();i++){
        if(
                h->channel == (*i)->getImage().getChannelNumber() &&
                (h->x >= (*i)->getX()) && (h->x <= ((*i)->getX() + (*i)->getPixelWidth())) &&
                (h->y >= (*i)->getY()) && (h->y <= ((*i)->getY() + (*i)->getPixelHeight()))                
        ){
            return true;
        }
    }
    
    return false;
}

genome* updateFitnesses(std::vector<genome*> & genomes,std::vector<SidescanFile*> & files,std::vector<std::vector<hit*> *> & hits){
    genome* bestFit = NULL;
    
    std::cerr << "[+] Updating fitness values" << std::endl;
    
    //for every genome
    for(auto g=genomes.begin();g!=genomes.end();g++){       
        
        if(!bestFit) bestFit=*g;
        
        //recompute fitness
        double fitness = 0.0;
        
        int truePositive   = 0;
        int precisionCount = 0;
        
        int recalled = 0;
        int recallCount = 0;
        
        //TODO : selector detector type
        //
        RoiDetector roiDetector(
                                (*g)->fastThreshold,
                                cv::FastFeatureDetector::TYPE_9_16,
                                false,
                                (*g)->dbscanEpsilon,
                                (*g)->dbscanMinPts,
                                (*g)->mserDelta,
                                (*g)->mserMinArea,
                                (*g)->mserMaxArea,
                                true
                               );                
        
        std::cout<<"file size : "<<files.size()<<"\n";
        //using each file
        for(unsigned int fileIdx=0;fileIdx<files.size();fileIdx++){
            //std::cout<<"fileIdx : "<<fileIdx<<std::endl;
            std::vector<InventoryObject*> detections; 
            SidescanFile *element = files[fileIdx];
            SidescanImage * image = element->getImages()[1];
            InventoryObject crabpot41(*image,600,1050,700,1150);
            detections.push_back(&crabpot41);
            //detections.push_back(image,600,1050,700,1150,"","");//point in detection
            //detections.push_back(image,500,1300,600,1400,"","");     
            //detections.push_back(**image,1,1,100,100,"",""); //point not in detections
            //detections.push_back(image,200,200,300,300); 
            
            
                //and each image
                /*
                for(auto i=files[fileIdx]->getImages().begin();i!=files[fileIdx]->getImages().end();i++){                
                    
                     //roiDetector.detect(**i, detections);
                
                }*/
                //std::cout<<"detection size: "<<detections.size()<<std::endl;
                // check if all hits have been detect
                for(auto detection=detections.begin();detection != detections.end(); detection++){
                    if(insideHits(*detection,* hits[fileIdx])){ 
                        std::cerr << "HIT" << std::endl;
                        recalled++;
                    }
                    
                    recallCount++;
                }
                
                //check if all detections have a hit
                for(auto h=hits[fileIdx]->begin(); h!=hits[fileIdx]->end(); h++){
                    if(insideDetections(*h,detections)){ 
                        std::cerr << "true positive" << std::endl;
                        truePositive++;
                    }
                    
                    
                    precisionCount++;
                }
                /*
                for(auto i=detections.begin();i!=detections.end();i++){
                    delete (*i);
                }*/
            detections.clear();
         }
        //compute fitness
        double precision = (truePositive > 0 && precisionCount > 0)?((double)truePositive/(double)precisionCount) * 100 : 0.0;
        double recall = (recalled > 0 && recallCount > 0)?((double)recalled/(double)recallCount)*100:0.0; 
        //TODO: add recall
        
        std::cerr << "[-] " << precision << "% / " << recall << " % (" << (*g)->fastThreshold << " " << (*g)->dbscanEpsilon << " " << (*g)->dbscanMinPts << " " << (*g)->mserDelta << " " << (*g)->mserMinArea << " " << (*g)->mserMaxArea << " )" << std::endl;
        
        fitness = precision + recall; 
        
        (*g)->fitness = fitness;
        
        if(bestFit->fitness < fitness)  bestFit= (*g);
    }
    
    return bestFit;
}


int main(int argc,char** argv){
    
    if(argc != 2){
        printUsage();
        return 1;
    }
    
    srand (time(NULL));    
    
    std::string directory = argv[1];
    
    std::vector<SidescanFile*>       files;
    std::vector<std::vector<hit*> *> hits;

    std::vector<genome*>             genomes;
    
    double lastFit = 0.0;
    
    genome * bestFit= NULL;    
    double fitThreshold   = 199.999;

    int nbGen = 0;    
    int genMaxCount = 1;
    
    try{
        load_SSS_Files(files,directory);
        load_HITS_Files(hits,directory);
        if(files.size() != hits.size()){
            throw std::runtime_error(" number of sidescan files and hits files should be equal");
        }
        else{
        
            initGenomes(genomes);
            while((!bestFit || bestFit->fitness < fitThreshold) && nbGen < genMaxCount){
                std::cerr << "[+] Generation " << nbGen << std::endl;
                
                bestFit = updateFitnesses(genomes,files,hits);
                break;
                std::cerr << "[-] Best fitness: " << bestFit->fitness << std::endl;
                
                if(bestFit->fitness > fitThreshold){
                    break;
                }
            }
            
            
        }
    }
    catch(Exception  *e){
    std::cerr << "Error: " << e->what() << std::endl;
    }
      
    return 0;
}
