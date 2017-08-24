//
//  ofxGiantImage.cpp
//
//  Created by Tim Knapen on 15/03/13.
//  www.timknapen.be
//

#include "ofxGiantImage.h"


//--------------------------------------------------------
ofxGiantImage::ofxGiantImage(){
    wtiles = htiles = width = height = 0;
    tileSize = 512; // ofImage uses more than the imagesize for the openGL texture, so maybe I should stay a bit under the ideal?
}

//--------------------------------------------------------
ofxGiantImage::~ofxGiantImage(){
    wtiles = htiles = 0;
    for (int i = 0; i < tiles.size(); i++) {
        tiles[i]->clear();
        delete tiles[i];
    }
    tiles.clear();
}

//--------------------------------------------------------
void ofxGiantImage::loadImage(string const & filePath){
    
	bool textureHackWasEnabled = ofIsTextureEdgeHackEnabled();
	// ofImage does a weird hack for the edges of openGL textures switch that off
	if (textureHackWasEnabled)
		ofDisableTextureEdgeHack();

    
    // load into a big image
    ofImage * img = new ofImage();
    img->setUseTexture(false);
    if(!img->loadImage(filePath)){
        cout << "I couldn't load the image from path: "<<filePath<<endl;
        return;
    }
	
	bool isGrayscale = true;
	bool hasAlpha = false;
	int bpp = img->getPixelsRef().getBytesPerPixel();
	
	int internalFormat = GL_RGB;
	
	switch (img->getImageType()) {
	case OF_IMAGE_COLOR:
		internalFormat = GL_RGB;
		break;
	case OF_IMAGE_COLOR_ALPHA:
		internalFormat = GL_RGBA;
		break;
	case OF_IMAGE_GRAYSCALE:
		internalFormat = GL_LUMINANCE;
		break;
	default:
		cout << "unrecognized image type: " << img->getImageType() << " - failed to generate image" << endl;
		img->clear();
		delete img;
		if (textureHackWasEnabled)
			ofEnableTextureEdgeHack();
		return;
	}

	
    
    width = img->getWidth();
    height = img->getHeight();
    cout << "Loaded source image of "<<width<<" by "<<height<<" pixels"<<endl;
    wtiles = (int) ceilf(width/(float)tileSize);
    htiles = (int) ceilf(height/(float)tileSize);
    
    unsigned char * tilepix = new unsigned char[bpp*tileSize*tileSize];
    
    // now start making tiles!
    for(int tile_y = 0; tile_y < htiles; tile_y++){
        for (int tile_x = 0; tile_x < wtiles; tile_x++) {
            
            int x1 = tileSize * tile_x;
            int y1 = tileSize * tile_y;
            int tileWidth = MIN(tileSize, width - x1); // only necessary for the tiles on the far right
            int tileHeight = MIN(tileSize, height - y1); // only necessary for the tiles on the bottom row
            ofTexture * tile = new ofTexture();
			
			tile->allocate(tileWidth, tileHeight, internalFormat);
            
			// copy pixels into tilepix row by row
			for (int iy = 0; iy < tileHeight; iy++) {
				auto line = img->getPixelsRef().getLine(y1 + iy).asPixels();
				unsigned char * lineData = line.getData();
				memcpy(
					&(tilepix[bpp * (iy * tileWidth)]),
					&(lineData[x1 * bpp]),
					bpp * tileWidth * sizeof(unsigned char));
			}
			
			tile->loadData(tilepix, tileWidth, tileHeight, internalFormat);
            tiles.push_back(tile);
        }
    }

	delete tilepix;
    
    if( wtiles * htiles != tiles.size()){
        cout << "WARNING the tiler created a grid of "<<(wtiles*htiles)<<" tiles but the vector has "<<tiles.size()<<" tiles!"<<endl;
    }else{
        cout << "the tiler created "<< wtiles <<" by "<<htiles<<" tiles ( = "<< (wtiles * htiles) <<" )"<<endl;
    }
    
    // destroy the source image
    img->clear();
    delete img;
    
    // switch the ofImage texture hack back on
	if (textureHackWasEnabled)
		ofEnableTextureEdgeHack();

}

//------------------------------------------------
void ofxGiantImage::draw(float x, float y, float w, float h){
    
    int x0 = floorf(x/(float)tileSize);
    int y0 = floorf(y/(float)tileSize);
    x0 = MAX(0, x0);
    y0 = MAX(0, y0);
    int maxix = ceilf( (x + w) / (float) tileSize );
    int maxiy = ceilf( (y + h) / (float) tileSize );
    maxix = MIN( wtiles, maxix);
    maxiy = MIN( htiles, maxiy);
    
    ofPushMatrix();
    ofTranslate( - x,  -y);
    //ofScale(ofGetWidth()/width, ofGetWidth()/width);
    for(int ix = x0 ; ix < maxix; ix ++ ){
        for(int iy = y0 ; iy < maxiy; iy++){
            tiles[ ix + (iy * wtiles)]->draw( ix * tileSize, iy * tileSize);
        }
    }
    ofPopMatrix();
}

//------------------------------------------------
void ofxGiantImage::drawBounds(float x, float y, float w, float h){
    ofPushStyle();
    ofEnableAlphaBlending();
    //ofSetColor(0, 0, 0, 10);
    ofSetLineWidth(3);
    ofNoFill();
    
    int x0 = floorf(x/(float)tileSize);
    int y0 = floorf(y/(float)tileSize);
    x0 = MAX(0, x0);
    y0 = MAX(0, y0);
    int maxix = ceilf( (x + w) / (float) tileSize );
    int maxiy = ceilf( (y + h) / (float) tileSize );
    maxix = MIN( wtiles, maxix);
    maxiy = MIN( htiles, maxiy);
	ofTexture * tile;

    ofPushMatrix();
    ofTranslate( - x,  -y);
    for(int ix = x0 ; ix < maxix; ix ++ ){
        for(int iy = y0 ; iy < maxiy; iy++){
			tile = tiles[ ix + (iy * wtiles)];
            ofPushMatrix();
            ofTranslate(ix * tileSize, iy * tileSize);
            ofRect(0, 0,  tile->getWidth(),tile->getHeight());
            

            ofPopMatrix();
        }
    }
    ofPopMatrix();
    
    


    ofPushMatrix();
	
	ofSetLineWidth(1);
    ofSetColor(100,100,100,70);
    ofTranslate(20, 20);
    ofScale(800/width, 800/width);
    for(int ix = 0 ; ix < wtiles; ix ++ ){
        for(int iy = 0 ; iy < htiles; iy++){
            ofRect(ix * tileSize, iy * tileSize,  tiles[ ix + (iy * wtiles)]->getWidth(), tiles[ ix + (iy * wtiles)]->getHeight());
        }
    }
    
    ofSetColor(0, 255, 0);

    for(int ix = x0 ; ix < maxix; ix ++ ){
        for(int iy = y0 ; iy < maxiy; iy++){
            
            ofRect(ix * tileSize, iy * tileSize,  tiles[ ix + (iy * wtiles)]->getWidth(), tiles[ ix + (iy * wtiles)]->getHeight());
        }
    }
    ofPopMatrix();
    ofDisableAlphaBlending();
    
    ofPopStyle();

}
