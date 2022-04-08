/*
* Copyright 2021 © Centre Interdisciplinaire de développement en Cartographie des Océans (CIDCO), Tous droits réservés
*/

/*
* \author Guillaume Labbe-Morissette
*/

#ifndef MAIN_CPP
#define MAIN_CPP

#include "../thirdParty/MBES-lib/src/datagrams/DatagramParserFactory.hpp"
#include <iostream>
#include <vector>
#include <string>
#include "opencv2/opencv.hpp"
#include <cmath>
#include <limits>

/**Writes the usage information about the datagram-list*/

void printUsage(){
	std::cerr << "\n\
	NAME\n\n\
	sidescan-dump - Dumps sidescan data of an image file\n\n\
	SYNOPSIS\n \
	sidescan-dump file\n\n\
	DESCRIPTION\n\n \
	Copyright 2021 © Centre Interdisciplinaire de développement en Cartographie des Océans (CIDCO), All rights reserved" << std::endl;
	exit(1);
}

/*!
* \brief Datagram Printer class
*
* Extends DatagramEventHandler.
*/
class SidescanGlcmCategorizer : public DatagramEventHandler{
public:
    
	/**
	* Creates a SidescanDataDumper
	*/
    
	SidescanGlcmCategorizer(std::string & filename):filename(filename){

	}

	/**Destroys the SidescanDataDumper*/
	~SidescanGlcmCategorizer(){

	}

        void processSidescanData(SidescanPing * ping){
            
            if(channels.size() < ping->getChannelNumber()+1){
                std::vector<SidescanPing*> * u = new std::vector<SidescanPing*>();
                channels.push_back(u);
            }

            channels[ping->getChannelNumber()]->push_back(ping);
        }
        
        
	void computeGlcm(cv::Mat &img, int windowSize, std::vector<std::vector<std::vector<double>>> &processedFeatures){
    
    	if(windowSize % 2 == 0){
    		std::cerr<<"windowSize must be odd !\n";
    		return;
    	}
    	else{
    		std::vector<double> features;
    		std::vector<std::vector<double>> ys;
    		features.reserve(7);
    		ys.reserve(img.rows);
    		
    		cv::MatIterator_<cv::Vec3b> itY, endY, itX, endX, winItY, winItX, wEndY, wEndX;
    		
    		int row = windowSize/2;
			int col = windowSize/2;
					
			itY = img.begin<cv::Vec3b>() + (windowSize/2 * img.cols) + windowSize/2;
			endY = img.end<cv::Vec3b>() - ((windowSize/2 * img.cols) - windowSize/2);
			
			//for every pixel in image
			for(itY, endY; itY != endY; itY += img.cols){
				//std::cout<<"image loop: "<<row - windowSize/2<<"\n";
				row++; //nedded for shade and prominence feature
				col = windowSize/2;
				for(itX = itY, endX = itX + img.cols - windowSize; itX != endX; itX++){
					col++;
					//compute glcm over window
					cv::Mat glcm(cv::Size(256,256), CV_64F, cv::Scalar(0)); // XXX
					int count = 0;
									
					winItY = itX - ((windowSize/2 - 1) * img.cols) - (windowSize/2 - 1);
					wEndY = itX + ((windowSize/2) * img.cols) - (windowSize/2 - 1);
					
					//for pixel in window
					for(winItY, wEndY; winItY != wEndY; winItY += img.cols){
						for(winItX = winItY, wEndX = winItX + (windowSize - 2); winItX != wEndX; winItX++ ){
							
							int value = int((*winItX)[0]);
							int valueUp = int((*(winItX - img.cols))[0]);
							int valueDown = int((*(winItX + img.cols))[0]);
							int valueLeft = int((*(winItX - 1))[0]);
							int valueRight = int((*(winItX + 1))[0]);
							//std::cout<<"values: "<<value<<" "<<valueUp<<" "<<valueDown<<" "<<valueLeft<<" "<<valueRight<< "\n";
							
							glcm.at<double>(valueUp, value, 0)++;
							glcm.at<double>(valueDown, value, 0)++;
							glcm.at<double>(valueLeft, value, 0)++;
							glcm.at<double>(valueRight, value, 0)++;
							count += 4;
							
						}
					}
					//std::cout<< glcm<<"\n\n";
					//std::this_thread::sleep_for (std::chrono::seconds(2));
					
					cv::Mat transposedGlcm(cv::Size(256,256), CV_64F, cv::Scalar(0));
					cv::transpose(glcm, transposedGlcm);
					cv::Mat normalizedGlcm = (glcm + transposedGlcm)/(count*2);
					//std::cout<< normalizedGlcm<<"\n\n";
					//std::this_thread::sleep_for (std::chrono::seconds(2));
					
					double energy = 0;
					double contrast = 0;
					double homogeneity = 0;
					double entropy = 0;
					double correlation = 0;
					double shade = 0;
					double prominence = 0;
					double glcmMean = 0;
					double sigma = 0;
					double squaredVarianceIntensity = 0;
					double A = 0;
					double B = 0;
					
					//auto start = high_resolution_clock::now();
					int r =0, c = 0;
					cv::MatIterator_<cv::Vec3d> itY2, endY2, itX2, endX2;
					for(itY2 =normalizedGlcm.begin<cv::Vec3d>(), endY2 =normalizedGlcm.end<cv::Vec3d>() ; itY2 != endY2; itY2+=normalizedGlcm.cols){
						r++;
						c=0;
						for(itX2 = itY2, endX2 = itX2 + normalizedGlcm.cols; itX2 != endX2; itX2++){
							c++;
							double pij = (*itX)[0];
							//std::cout<<"intensity: "<<intensity<<"\n";
							//std::cout<<"pij: "<<pij<<"\n";

							if(pij != 0){
								homogeneity += pij/(1.0+((c-r)*(c-r)));
								energy += pij * pij;
								contrast += (c-r)*(c-r)*pij;
								entropy += -(log(pij)*pij);
							}
							else{
								break;
								// += 0 to all feature
							}

						}
						
					}
					
					for(itY2 =normalizedGlcm.begin<cv::Vec3d>(), endY2 =normalizedGlcm.end<cv::Vec3d>() ; itY2 != endY2; itY2+=normalizedGlcm.cols){
						for(itX2 = itY2, endX2 = itX2 + normalizedGlcm.cols; itX2 != endX2; itX2++){
							double pij = (*itX)[0];
							double intensity = double((*itX)[0]);
							if(pij != 0){
								glcmMean += pij * intensity;
							}
						}
					}
					
					for(itY2 =normalizedGlcm.begin<cv::Vec3d>(), endY2 =normalizedGlcm.end<cv::Vec3d>() ; itY2 != endY2; itY2+=normalizedGlcm.cols){
						for(itX2 = itY2, endX2 = itX2 + normalizedGlcm.cols; itX2 != endX2; itX2++){
							double pij = (*itX)[0];
							double intensity = double((*itX)[0]);
							if(pij != 0){
								squaredVarianceIntensity += pij*((intensity - glcmMean)*(intensity - glcmMean));
							}
						}
					}
					/*
					auto stop = high_resolution_clock::now();
					auto duration = duration_cast<microseconds>(stop - start);
					double time = duration.count()/1000;
					std::cout<<"norm glcm loop 1"<<time<<"\n";
					
					start = high_resolution_clock::now();
					*/
					for(itY2 =normalizedGlcm.begin<cv::Vec3d>(), endY2 =normalizedGlcm.end<cv::Vec3d>() ; itY2 != endY2; itY2+=normalizedGlcm.cols){
						for(itX2 = itY2, endX2 = itX2 + normalizedGlcm.cols; itX2 != endX2; itX2++){
							double pij = (*itX)[0];
							if(pij != 0){
								//XXX handle very small squaredVarianceIntensity
								if(squaredVarianceIntensity < 1.0e-15){
									squaredVarianceIntensity = 1.0e-15;
								}
								correlation += pij*(((c-glcmMean)*(r-glcmMean))/squaredVarianceIntensity);

							}
						}
					}
					
					for(itY2 =normalizedGlcm.begin<cv::Vec3d>(), endY2 =normalizedGlcm.end<cv::Vec3d>() ; itY2 != endY2; itY2+=normalizedGlcm.cols){
						for(itX2 = itY2, endX2 = itX2 + normalizedGlcm.cols; itX2 != endX2; itX2++){
							double pij = (*itX)[0];
							if(pij != 0){
								double intensity = (double)(*itX)[0];
								sigma = intensity - glcmMean; //XXX or sqrt(squaredVarianceIntensity);
									
								A=(pow((col+row)-2*glcmMean,3)*pij)/(pow(sigma, 3)*(sqrt(2*(1+correlation))));	
								if(A > 0){
									shade += pow(abs(A), 1/3);
								}
								else if(A<0){
									shade += pow(abs(A), 1/3) * -1 ;
								}
								
								
								B = (pow((col+row)- 2*glcmMean, 4)*pij)/(4* pow(sigma, 4)* pow(1+correlation, 2));
								if(B > 0){
									prominence += pow(abs(B), 1/4);
								}
								else if(B < 0){
									prominence += pow(abs(B), 1/4) * -1 ;
								}
							}
						}
					}
					
					/*
					stop = high_resolution_clock::now();
					duration = duration_cast<microseconds>(stop - start);
					time = duration.count()/1000;
					std::cout<<"norm glcm loop 2"<<time<<"\n";
					*/
					//std::cout<<"features: "<< energy <<" "<< contrast <<" "<< homogeneity <<" "<< entropy <<" "<< correlation <<"\n";
					features.push_back(energy);
					features.push_back(contrast);
					features.push_back(homogeneity);
					features.push_back(entropy);
					features.push_back(correlation);
					features.push_back(shade);
					features.push_back(prominence);
					ys.push_back(features);
					features.clear();
				}
				
				processedFeatures.push_back(ys);
				ys.clear();
			}
    	}
	return;
    }
		
        
        
        void generateImages(){
            std::cerr << "Generating images for " << channels.size() << " channels" << std::endl;

            for(unsigned int i=0;i<channels.size();i++){
                std::stringstream ss;
                
                ss << filename <<  "-classes-" << i << ".jpg";
                std::cerr << "Channel " << i << std::endl;
                
                cv::Mat img(channels[i]->size(),channels[i]->at(0)->getSamples().size(), CV_64F,cv::Scalar(0));
                
                std::cerr << "Rows: " << channels[i]->size() << " Cols: " << channels[i]->at(0)->getSamples().size() << std::endl;                 
                
                for(unsigned int j=0;j<channels[i]->size();j++){ //j indexes rows
                    for(unsigned int k=0;k<channels[i]->at(j)->getSamples().size();k++){ //k indexes cols 
                        img.at<double>(j, k, 0) = channels[i]->at(j)->getSamples().at(k);
                    }
                }
                
                cv::normalize(img,img,200000,0);
		        cv::Mat I;
		        img.convertTo(I, CV_8UC1);
		
                //post-process greyscale
                
                equalizeHist(I,I);
                fastNlMeansDenoising(I,I);  
              
				//TODO: make this a command-line parameter
				// blur(I,I,cv::Size(2,2));
                
                int start = filename.rfind("/");
                std::string outputFilename= filename.substr(filename.rfind("/") +1, 
                											filename.size() - (filename.rfind(".")-2));
				
				outputFilename += "_" + std::to_string(i) + ".haralick";
                
                // output classes
                int windowSize = 31; // TODO make it a param
                cv::Mat classes;
                std::vector<std::vector<std::vector<double>>> processedFeatures;
                //processedFeatures.reserve(I.cols/windowSize);
                computeGlcm(I, windowSize, processedFeatures);
                
                
                std::ofstream outputFile;
				outputFile.open (outputFilename);
				for(auto &v:processedFeatures){
					for(auto &feature: v){
						outputFile<< feature[0]<<" "<< feature[1]<<" "<< feature[2]<<" "
								<< feature[3]<<" "<< feature[4]<<" "<< feature[5]<<" "<< feature[6]<<"\n";
					}
				}
				
				//imwrite(ss.str(), classes);
            }
        }
        
private:
    std::string filename;
    
    std::vector<  std::vector<SidescanPing * > * > channels;
};

/**
* Declares the parser depending on argument received
*
* @param argc number of argument
* @param argv value of the arguments
*/
int main (int argc , char ** argv ){
	DatagramParser *    parser      = NULL;

        #ifdef __GNU__
	setenv("TZ", "UTC", 1);
	#endif
	#ifdef _WIN32
	putenv("TZ");
	#endif

	if(argc != 2){
		printUsage();
	}

	std::string fileName(argv[1]);

	try{
		SidescanGlcmCategorizer  sidescan(fileName);

		std::cerr << "[+] Decoding " << fileName << std::endl;

		parser = DatagramParserFactory::build(fileName,sidescan);

		parser->parse(fileName);
                
		sidescan.generateImages();
	}
	catch(std::exception * e){
		std::cerr << "[-] Error while parsing " << fileName << ": " << e->what() << std::endl;
	}


	if(parser) delete parser;
	
	
}


#endif
