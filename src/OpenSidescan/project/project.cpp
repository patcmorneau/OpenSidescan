#include "project.h"

#include <cstring>
#include <vector>

#include <QFile>
#include <QtXml>
#include <QPixmap>

#include "sidescan/sidescanimager.h"
#include "utilities/qthelper.h"





Project::Project()
{

}

Project::~Project(){
    mutex.lock();

    for(auto i=files.begin();i!=files.end();i++){
        delete (*i);
    }

    mutex.unlock();
}

void Project::addFile(SidescanFile * newFile){
    mutex.lock();

    files.push_back(newFile);

    mutex.unlock();
}

void Project::removeFile(SidescanFile * file){
    mutex.lock();

    auto iter = std::find( files.begin(), files.end(), file );

    if ( iter != files.end() )
    {
        files.erase( iter );
    }

    mutex.unlock();
}

void Project::read(std::string & filename){
    QFile file(QString::fromStdString(filename));
    file.open(QIODevice::ReadOnly);

    QXmlStreamReader xml(&file);

    mutex.lock();

    std::string currentImage;
    SidescanFile * currentFile= nullptr;

    while(!xml.atEnd()){

        //Read through file
        switch(xml.readNext()){
            case QXmlStreamReader::StartElement:
                std::string name = xml.name().toString().toStdString();

                //Handle different element types
                if(strncmp(name.c_str(),"Project",7)==0) {
                    //read lever arm
                    QStringRef leverArmX = xml.attributes().value(QString::fromStdString("leverArmX"));
                    QStringRef leverArmY = xml.attributes().value(QString::fromStdString("leverArmY"));
                    QStringRef leverArmZ = xml.attributes().value(QString::fromStdString("leverArmZ"));

                    if(leverArmX.isEmpty() || leverArmY.isEmpty() || leverArmZ.isEmpty() ) {
                        //no lever arm, old project file
                        antenna2TowPointLeverArm << 0 ,0 ,0;
                    } else {
                        antenna2TowPointLeverArm << leverArmX.toDouble(), leverArmY.toDouble(), leverArmZ.toDouble();
                    }
                } else if(strncmp(name.c_str(),"File",4)==0){
                    //Sidescan file
                    std::string filename = xml.attributes().value(QString::fromStdString("filename")).toString().toStdString();

                    SidescanImager imager;
                    DatagramParser * parser = DatagramParserFactory::build(filename,imager);
                    parser->parse(filename);
                    currentFile = imager.generate(filename, antenna2TowPointLeverArm);

                    files.push_back(currentFile);

                    currentImage = "";

                    delete parser;
                }
                else if(strncmp(name.c_str(),"Image",5)==0){
                    //Sidescan image
                    currentImage=xml.attributes().value(QString::fromStdString("channelName")).toString().toStdString();
                }
                else if(strncmp(name.c_str(),"Object",5)==0){
                    //Inventory Objects
                    if(currentFile){
                        for(auto i = currentFile->getImages().begin();i!=currentFile->getImages().end();i++){
                            if(strncmp((*i)->getChannelName().c_str(),currentImage.c_str(),currentImage.size())==0){
                                //instanciate object
                                int x                   = std::stoi(xml.attributes().value(QString::fromStdString("x")).toString().toStdString());
                                int y                   = std::stoi(xml.attributes().value(QString::fromStdString("y")).toString().toStdString());
                                int pixelWidth          = std::stoi(xml.attributes().value(QString::fromStdString("pixelWidth")).toString().toStdString());
                                int pixelHeight         = std::stoi(xml.attributes().value(QString::fromStdString("pixelHeight")).toString().toStdString());
                                std::string name        = xml.attributes().value(QString::fromStdString("name")).toString().toStdString();
                                std::string description = xml.attributes().value(QString::fromStdString("description")).toString().toStdString();

                                InventoryObject * object = new InventoryObject(*(*i),x,y,pixelWidth,pixelHeight,name,description);
                                (*i)->getObjects().push_back(object);
                            }
                        }

                    }
                    else{
                        //No file...wtf
                        std::cerr << "Malformed Project File: No file associated with object" << std::endl;
                    }
                }
            break;
        }
    }

    mutex.unlock();
}

void Project::write(std::string & filename){
    mutex.lock();

    QFile file(QString::fromStdString(filename));
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("Project");
    xmlWriter.writeAttribute(QString::fromStdString("leverArmX"),QString::number(antenna2TowPointLeverArm[0]));
    xmlWriter.writeAttribute(QString::fromStdString("leverArmY"),QString::number(antenna2TowPointLeverArm[1]));
    xmlWriter.writeAttribute(QString::fromStdString("leverArmZ"),QString::number(antenna2TowPointLeverArm[2]));

    //prepare to compute relative file paths
    QFileInfo fileInfo(QCoreApplication::applicationFilePath());
    QDir projectDir(fileInfo.canonicalPath());
    //qDebug()<<fileInfo.canonicalPath();

    for(auto i=files.begin();i!=files.end();i++){
        xmlWriter.writeStartElement("File");

        QString sssRelativePath = projectDir.relativeFilePath(QString::fromStdString((*i)->getFilename()));

        //qDebug()<<sssRelativePath<<"\n";

        xmlWriter.writeAttribute(QString::fromStdString("filename"),sssRelativePath);

        for(auto j=(*i)->getImages().begin();j!=(*i)->getImages().end();j++){
            //TODO: write objects
            xmlWriter.writeStartElement("Image");

            xmlWriter.writeAttribute(QString::fromStdString("channelName"), QString::fromStdString((*j)->getChannelName()) );

            for(auto k = (*j)->getObjects().begin(); k != (*j)->getObjects().end(); k++){
                xmlWriter.writeStartElement("Object");

                xmlWriter.writeAttribute(QString::fromStdString("x"),           QString::fromStdString( std::to_string((*k)->getX())            ) );
                xmlWriter.writeAttribute(QString::fromStdString("y"),           QString::fromStdString( std::to_string((*k)->getY())            ) );
                xmlWriter.writeAttribute(QString::fromStdString("pixelWidth"),  QString::fromStdString( std::to_string((*k)->getPixelWidth())   ) );
                xmlWriter.writeAttribute(QString::fromStdString("pixelHeight"), QString::fromStdString( std::to_string((*k)->getPixelHeight())  ) );
                xmlWriter.writeAttribute(QString::fromStdString("name"),        QString::fromStdString( (*k)->getName()                         ) );
                xmlWriter.writeAttribute(QString::fromStdString("description"), QString::fromStdString( (*k)->getDescription()                  ) );

                xmlWriter.writeEndElement();
            }

            xmlWriter.writeEndElement();
        }

        xmlWriter.writeEndElement();
    }

    xmlWriter.writeEndElement();

    file.close();

    mutex.unlock();
}


void Project::exportInventoryAsKml(std::string & filename){
    QFile file(QString::fromStdString(filename));
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("kml");
    xmlWriter.writeNamespace(QString::fromStdString("http://www.opengis.net/kml/2.2"));
    xmlWriter.writeStartElement("Document");

    mutex.lock();

    for(auto i=files.begin();i!=files.end();i++){
        for(auto j=(*i)->getImages().begin();j!=(*i)->getImages().end();j++){
            for(auto k=(*j)->getObjects().begin();k!=(*j)->getObjects().end();k++){

                if((*k)->getPosition()){
                    xmlWriter.writeStartElement("Placemark");

                    //name
                    xmlWriter.writeStartElement("name");
                    xmlWriter.writeCharacters(QString::fromStdString((*k)->getName()));
                    xmlWriter.writeEndElement();

                    //description
                    xmlWriter.writeStartElement("description");
                    xmlWriter.writeCDATA(QString::fromStdString((*k)->getDescription()));
                    xmlWriter.writeEndElement();

                    //Point coordinates
                    std::stringstream ss;
                    ss << std::setprecision(15);
                    ss << (*k)->getPosition()->getLongitude() << "," << (*k)->getPosition()->getLatitude() ;

                    xmlWriter.writeStartElement("Point");

                    xmlWriter.writeStartElement("coordinates");
                    xmlWriter.writeCharacters(QString::fromStdString(ss.str()));
                    xmlWriter.writeEndElement();

                    xmlWriter.writeEndElement();


                    xmlWriter.writeEndElement();
                }
            }
        }
    }

    mutex.unlock();

    xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();

    file.close();
}

void Project::exportInventoryAsCsv(std::string & filename){
    std::ofstream outFile;
    outFile.open( filename, std::ofstream::out );

    if( outFile.is_open() ){
        outFile << "name" << "," << "description" << "," << "longitude" << "," << "latitude" << "\n";

        outFile << std::fixed << std::setprecision(15);

        mutex.lock();

        for(auto i = files.begin(); i != files.end(); ++i){
            for(auto j=(*i)->getImages().begin();j!=(*i)->getImages().end();j++){
                for(auto k=(*j)->getObjects().begin();k!=(*j)->getObjects().end();k++){
                    Position * pos = (*k)->getPosition();
                    outFile << "\"" << (*k)->getName() << "\"," << "\"" << (*k)->getDescription() << "\","  << pos->getLongitude() << "," << pos->getLatitude() << "\n";
                }
            }
        }

        mutex.unlock();

        outFile.close();
    }
}

void Project::exportInventoryAsHits(std::string & path){

    for(auto i = files.begin(); i != files.end(); ++i){

        std::string filename = (*i)->getFilename();
        QFileInfo fileInfo(QString::fromStdString(filename));
        QString FileName = fileInfo.fileName();
        QFileInfo pathInfo(QString::fromStdString(path));
        pathInfo.setFile(QString::fromStdString(path),FileName);
        QString filePath = pathInfo.filePath();
        std::string Path = filePath.toStdString();
        Path.append(".hits");

        std::ofstream outFile;
        outFile.open( Path, std::ofstream::out );
        if( outFile.is_open() ){
            mutex.lock();
            for(auto j=(*i)->getImages().begin();j!=(*i)->getImages().end();j++){
                for(auto k=(*j)->getObjects().begin();k!=(*j)->getObjects().end();k++){
                    outFile<<(*j)->getChannelNumber()<< " "   << (*k)->getXCenter() << " " << (*k)->getYCenter() << "\n";
                }
            }
            mutex.unlock();
            outFile.close();
            Path = "";
        }
        else{
            std::cerr<<"cant create new file"<<std::endl;
        }
    }
}


//void Project::saveObjectImages( const QString & folder )
void Project::saveObjectImages( const QString & absolutePath, const QString & fileNameWithoutExtension )
{
    QString fileNameHTML = absolutePath + "/" + fileNameWithoutExtension + ".html";

    QFile file( fileNameHTML );

    if( file.open(QIODevice::WriteOnly) )
    {
        QXmlStreamWriter xmlWriter(&file);

        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();

        xmlWriter.writeDTD( "<!DOCTYPE html>" );

        xmlWriter.writeStartElement("html");

        // Style

        xmlWriter.writeStartElement("head");
        xmlWriter.writeStartElement("style");

        xmlWriter.writeCharacters( "table, th, td {\n" );
        xmlWriter.writeCharacters( "  border: 1px solid black;\n" );
        xmlWriter.writeCharacters( "  border-collapse: collapse;\n" );
        xmlWriter.writeCharacters( "}\n" );
        xmlWriter.writeCharacters( "th, td {\n" );
        xmlWriter.writeCharacters( "  padding: 5px;\n" );
        xmlWriter.writeCharacters( "}\n" );
        xmlWriter.writeCharacters( "th {\n" );
        xmlWriter.writeCharacters( "  text-align: left;\n" );
        xmlWriter.writeCharacters( "}\n" );

        xmlWriter.writeEndElement(); // style
        xmlWriter.writeEndElement(); // head

        // Body

        xmlWriter.writeStartElement("body");

        xmlWriter.writeStartElement("h2"); // Left-align Headings
        xmlWriter.writeCharacters( "Objects" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("p");
        xmlWriter.writeCharacters( "List of objects" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement( "table style=\"width:100%\"" );

        // Table header
        xmlWriter.writeStartElement("tr");

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "Target" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "Name" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "File" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "Channel" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "Longitude" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "Latitude" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "Width (m)" );
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("th");
        xmlWriter.writeCharacters( "Height (m)" );
        xmlWriter.writeEndElement();


        xmlWriter.writeEndElement(); // tr

        mutex.lock();

        for(auto i = files.begin(); i != files.end(); ++i){

            for(auto j=(*i)->getImages().begin();j!=(*i)->getImages().end();j++){

                for(auto k=(*j)->getObjects().begin();k!=(*j)->getObjects().end();k++){

                    // Copy the part of the cv::Mat with the object into a new cv::Mat
                    cv::Mat objectMat ( (*k)->getPixelHeight() ,(*k)->getPixelWidth(), CV_8UC1 );
                    (*j)->getImage()( cv::Rect( (*k)->getX(), (*k)->getY(), (*k)->getPixelWidth(), (*k)->getPixelHeight() ) ).copyTo( objectMat );

                    // Find filename that does not already exist
                    QString objectName = QString::fromStdString( (*k)->getName() );

                    QString fileExtension = "png";

                    QString objectImageFileName = objectName + "." + fileExtension;

                    QString objectImageFileNameWithPath = absolutePath + "/" + fileNameWithoutExtension + "/" + objectImageFileName;

                    QFileInfo fileInfo( objectImageFileNameWithPath );

                    int count = 0;

                    while ( fileInfo.exists() ) {

                        objectImageFileName = objectName + "_" + QString::number( count ) + "." + fileExtension;
                        objectImageFileNameWithPath = absolutePath + "/" + fileNameWithoutExtension + "/" + objectImageFileName;
                        fileInfo.setFile( objectImageFileNameWithPath );
                        count++;
                    }

                    // Save pixmap
                    cv::imwrite(objectImageFileNameWithPath.toStdString(),objectMat);

                    xmlWriter.writeStartElement("tr");

                    //Image
                    xmlWriter.writeStartElement("td");

                    QString imageString = "img src=\"" + fileNameWithoutExtension + "/" + objectImageFileName + "\" alt=\"" + objectImageFileName + "\"";
                    xmlWriter.writeStartElement( imageString );
                    xmlWriter.writeEndElement(); // imageString

                    xmlWriter.writeEndElement(); // td

                    //Target name

                    xmlWriter.writeStartElement("td");
                    xmlWriter.writeCharacters( objectName );
                    xmlWriter.writeEndElement();

                    //File name
                    QFileInfo sidescanFileInfo( QString::fromStdString((*i)->getFilename()) );
                    QString filenameWithoutPath = sidescanFileInfo.fileName();

                    xmlWriter.writeStartElement("td");
                    xmlWriter.writeCharacters( filenameWithoutPath );
                    xmlWriter.writeEndElement();

                    //Channel
                    xmlWriter.writeStartElement("td");
                    xmlWriter.writeCharacters(  QString::fromStdString((*j)->getChannelName()) );
                    xmlWriter.writeEndElement();

                    //Longitude / Latitude

                    xmlWriter.writeStartElement("td");
                    xmlWriter.writeCharacters( ((*k)->getPosition()) ? QString::number((*k)->getPosition()->getLongitude(), 'f', 15) : "N/A" );
                    xmlWriter.writeEndElement();

                    xmlWriter.writeStartElement("td");
                    xmlWriter.writeCharacters( ((*k)->getPosition()) ? QString::number((*k)->getPosition()->getLatitude(), 'f', 15) : "N/A" );
                    xmlWriter.writeEndElement();

                    //Width
                    xmlWriter.writeStartElement("td");
                    xmlWriter.writeCharacters( ((*k)->getWidth() > 0) ?  QString::number( (*k)->getWidth(), 'f', 3) : "N/A" );
                    xmlWriter.writeEndElement();

                    //Height
                    xmlWriter.writeStartElement("td");
                    xmlWriter.writeCharacters( ((*k)->getHeight() > 0) ? QString::number( (*k)->getHeight(), 'f', 3) : "N/A" );
                    xmlWriter.writeEndElement();

                    xmlWriter.writeEndElement(); // tr
                }
            }
        }

        mutex.unlock();

        xmlWriter.writeEndDocument(); // Closes all remaining open start elements and writes a newline.

        file.close();
    }

    mutex.unlock();
}

unsigned long Project::getFileCount()
{
    unsigned long count = 0;

    mutex.lock();

    count = files.size();

    mutex.unlock();

    return count;
}

unsigned long Project::getObjectCount()
{
    unsigned long count=0;

    mutex.lock();

    for(auto i = files.begin();i != files.end();i++){
        for(auto j = (*i)->getImages().begin(); j != (*i)->getImages().end();j++){
            count += (*j)->getObjects().size();
        }
    }

    mutex.unlock();

    return count;
}




bool Project::containsFile(std::string & filename){
    bool res = false;

    mutex.lock();

    for(auto i=files.begin();i!=files.end();i++){
        if(strcmp((*i)->getFilename().c_str(),filename.c_str()) == 0){
            res=true;
            break;
        }
    }

    mutex.unlock();

    return res;
}

void Project::exportInventory4Yolo(std::string & path){
    for(auto i = files.begin(); i != files.end(); ++i){      
        for(auto j=(*i)->getImages().begin();j!=(*i)->getImages().end();j++){
            //int image_count =0;
            for(unsigned int index = 0; index < (*j)->getObjects().size(); index++){
                auto k = (*j)->getObjects().at(index);
                cv::Mat image = (*j)->getImage();                           //get image
                cv::Size image_dimension = image.size();
                int height = image_dimension.height;                        //get image height
                int width = image_dimension.width;

                //Here we are cropping arround the crabtrap 640x640 -> default resolution in yolov5
                int crop_start_y = 0;
                int crop_end_y = 0;
                int crop_start_x = 0;
                int crop_end_x = 0;
                //delta's are relative to the file
                int top_delta_y = k->getY();
                int bot_delta_y = height - 640 - k->getY();
                int left_delta_x = k->getX();
                int right_delta_x = width - 640 - k->getX();

                int x_direction = 0;
                int y_direction = 0;

                //determining where to start cropping process
                if(top_delta_y < bot_delta_y){
                    crop_start_y = k->getY();
                    crop_end_y = k->getY() + 640;
                    std::cout<<"if 1 \n crop start y :"<< crop_start_y
                             <<"  crop end y :"<<crop_end_y<<"\n";
                    if(crop_end_y > height){
                        int temp_delta = crop_end_y - height;
                        crop_start_y = k->getY() - temp_delta;
                        crop_end_y = height;
                        std::cout<<"if 1.5 \n crop start y :"<< crop_start_y
                                 <<"  crop end y :"<<crop_end_y<<"\n";
                    }
                    y_direction = -40;
                }
                else {
                    crop_start_y = (k->getY() + k->getPixelHeight()) - 640;
                    crop_end_y = crop_start_y + 640;
                    std::cout<<"if 2 \n crop start y :"<< crop_start_y
                             <<"  crop end y :"<<crop_end_y<<"\n";
                    if(crop_start_y < 0){
                        crop_end_y = crop_end_y - crop_start_y; // -- = +
                        crop_start_y = 0;
                        std::cout<<"if 2.5 \n crop start y :"<< crop_start_y
                                 <<"  crop end y :"<<crop_end_y<<"\n";
                    }
                    y_direction = 40;
                }

                if(left_delta_x < right_delta_x){
                    crop_start_x = k->getX();
                    crop_end_x = k->getX() + 640;
                    std::cout<<"if 3 \n crop start x :"<< crop_start_x
                             <<"  crop end x :"<<crop_end_x<<"\n";
                    if(crop_end_x > width){
                        int temp_delta = crop_end_x - width;
                        crop_start_x  = k->getX() - temp_delta;
                        crop_end_x = width;
                        std::cout<<"if 3.5 \n crop start x :"<< crop_start_x
                                 <<"  crop end x :"<<crop_end_x<<"\n";
                    }
                    x_direction = -40;
                }
                else{
                    crop_start_x = (k->getX()+ k->getPixelWidth()) - 640;
                    crop_end_x = crop_start_x + 640;
                    std::cout<<"if 4 \n crop start x :"<< crop_start_x
                             <<"  crop end x :"<<crop_end_x<<"\n";
                    if(crop_start_x < 0){
                        crop_end_x = crop_end_x - crop_start_x;
                        crop_start_x = 0;
                        std::cout<<"if 4.5 \n cropt start x :"<< crop_start_x
                                 <<"  crop end x :"<<crop_end_x<<"\n";
                    }
                    x_direction = 40;
                }
                //We are moving the cropping window to generate more than one picture of each object
                int start_pos_x = crop_start_x;
                int count = 0;
                //std::cout<<"\n\nimage dim: " << image_dimension << "\n";
                //std::cout<<"cropt start y :"<<crop_start_y
                //        <<"  crop end y :"<<crop_end_y<<"\n";
                while(crop_start_y >= 0 && crop_start_y <= k->getY() && crop_end_y <= height && crop_end_y >= k->getY() + k->getPixelHeight()){
                    //std::cout  <<"crop start x :"<<crop_start_x
                      //         <<"  crop end x :"<<crop_end_x << "\n";
                    while(crop_start_x >= 0 && crop_start_x <= k->getX() && crop_end_x <= width && crop_end_x >= k->getX() + k->getPixelWidth()){

                        std::string filename = (*i)->getFilename();             //get file name
                        QFileInfo fileInfo(QString::fromStdString(filename));
                        QString FileName = fileInfo.fileName();
                        QString chan = QString::number((*j)->getChannelNumber());  //get channel number
                        count++;
                        QString ImageCount = QString::number(count);
                        FileName = FileName + "-" + chan + "_" + ImageCount;
                        QString ImageName = FileName + ".png";
                        FileName = FileName + ".txt";

                        std::string FilePath = path + "/labels";
                        std::string ImagePath = path + "/images";

                        QFileInfo pathInfoFile(QString::fromStdString(FilePath));
                        QFileInfo pathInfoImage(QString::fromStdString(ImagePath));

                        pathInfoFile.setFile(QString::fromStdString(FilePath),FileName);
                        pathInfoImage.setFile(QString::fromStdString(ImagePath),ImageName);

                        QString filePath = pathInfoFile.filePath();
                        QString imagePath = pathInfoImage.filePath();

                        std::string FILEPATH = filePath.toStdString();
                        std::string IMAGEPATH = imagePath.toStdString();
                        std::cout<<FILEPATH<<"\n";
                        std::cout<<IMAGEPATH<<"\n";


                        cv::Mat new_image = image(cv::Range(crop_start_y, crop_end_y), cv::Range(crop_start_x, crop_end_x));
                        //needs to be fix
                        float norm_detect_xCenter = float((float(k->getPixelWidth()/2.0) + k->getX()) /640.0);
                        float norm_detect_yCenter = float(float(k->getPixelHeight()/2.0)/640.0);
                        float detect_norm_width = float(k->getPixelWidth()/640.0);
                        float detect_norm_height = float(k->getPixelHeight()/640.0);

                        //we can abstract this part, since it seems to be a recurrent thing to do
                        //inv_obj.write_to_file(vector<inv_obj> , filename)
                        std::ofstream outFile;
                        outFile.open( FILEPATH, std::ofstream::out );
                        if( outFile.is_open() ){
                            mutex.lock();
                            outFile <<"0 "<< norm_detect_xCenter <<" "<< norm_detect_yCenter <<" "
                                    << detect_norm_width <<" "<< detect_norm_height <<"\n";
                        }
                        else{
                            std::cerr<<"cant create new file \n"<<std::endl;
                        }
                        mutex.unlock();
                        outFile.close();
                        cv::imwrite(IMAGEPATH, new_image);

                        crop_start_x = crop_start_x + x_direction;
                        crop_end_x = crop_start_x + 640;
                    }
                    crop_start_x = start_pos_x;
                    crop_end_x = crop_start_x + 640;
                    crop_start_y = crop_start_y + y_direction;
                    crop_end_y = crop_start_y + 640;
                }
            }
        }
    }
}
