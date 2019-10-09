#include "ofApp.h"
#include "ofxSvg/src/ofxSvg.h"
ofxSVG svg;
vector<ofPath> paths;
//vector<ofPath> paths;
int animI = 0;
vector<ofVec2f> pts;
vector<vector<pair<int, bool> > > ptToPath;
vector<pair<int, int> > endPts;
int selectedPt = 0;
//--------------------------------------------------------------
void ofApp::setup(){

}

void order(string filename) {
	
	svg.load(filename);
	paths = svg.getPaths();
	ofSetWindowShape(svg.getWidth(), svg.getHeight());

	pts.clear();
	endPts.clear();
	ptToPath.clear();
	vector<pair<ofVec2f,ofVec2f> > endTans;

	for (int i = 0; i < paths.size(); ++i) {
		const ofPath & path = paths[i];
		vector<ofPolyline> outline = path.getOutline();
		//assume only one line per path
		if (outline.size() > 1) cout << "BADD" << endl;
		
		ofVec2f startPt = outline[0].getVertices().front();
		ofVec2f endPt = outline[0].getVertices().back();


		ofVec2f startTan = outline[0].getVertices()[2] - startPt;
		ofVec2f endTan = outline[0].getVertices()[outline[0].getVertices().size() - 3] - endPt;
		startTan.normalize();
		endTan.normalize();

		int startI = pts.size();
		for (int j = 0; j < pts.size(); ++j) {
			if (pts[j].squareDistance(startPt) < 1e-4) {
				startI = j;
				break;
			}
		}
		if (startI == pts.size()) pts.push_back(startPt);

		int endI = pts.size();
		for (int j = 0; j < pts.size(); ++j) {
			if (pts[j].squareDistance(endPt) < 1e-4) {
				endI = j;
				break;
			}
		}
		if (endI == pts.size()) pts.push_back(endPt);

		endPts.push_back(make_pair(startI, endI));
		endTans.push_back(make_pair(startTan, endTan));
		//cout << i << " " << startTan << " " << endTan << " " << outline[0].getVertices().size() << endl;
	}

	//get center and neighbors
	ptToPath.resize(pts.size());
	for (int i = 0; i < paths.size(); ++i) {
		auto & ends = endPts[i];
		ptToPath[ends.first].push_back(make_pair(i, false));
		ptToPath[ends.second].push_back(make_pair(i,true));
	}

	//merge edges
	/*
	vector<bool> merged(paths.size(), false);
	for (int i = 0; i < pts.size(); ++i) {
		auto & neighbors = ptToPath[i];
		if (neighbors.size() == 2) {
			int path1 = neighbors[0].first;
			int path2 = neighbors[1].first;
			bool flip1 = neighbors[0].second;
			bool flip2 = neighbors[1].second;
			int v1 = flip1 ? endPts[path1].first : endPts[path1].second;
			int v2 = flip2 ? endPts[path2].first : endPts[path2].second;
			neighbors.clear();

			paths[path1].append(paths[path2]);
			endPts[path1].first = v1;
			endPts[path1].second = v2;
			
			auto & neighbors2 = ptToPath[v2];
			for (int j = 0; j < neighbors2.size(); ++j) {
				if (neighbors2[j].first == path2) {
					neighbors2[j] = make_pair(path1, true);
				}
			}
			auto & neighbors3 = ptToPath[v1];
			for (int j = 0; j < neighbors3.size(); ++j) {
				if (neighbors3[j].first == path1) {
					neighbors3[j] = make_pair(path1, false);
				}
			}

			merged[path2] = true;
		}
	}
	*/
	//sort neighbors
	for (int i = 0; i < pts.size(); ++i) {
		auto & neighbors = ptToPath[i];
		if (neighbors.size() < 3) {
			cout << i<< " " << neighbors.size() << endl;
		}
		if (neighbors.size() > 0) {
			sort(neighbors.begin(), neighbors.end(), [&endTans, &i](pair<int,bool> i1, pair<int,bool> i2) -> bool {
				ofVec3f p1, p2, p3, p4;
				auto & l1 = endTans[i1.first];
				auto & l2 = endTans[i2.first];

				if (i1.second) {
					p1 = l1.second;
					p2 = l1.first;
				}
				else {
					p1 = l1.first;
					p2 = l1.second;
				}

				if (i2.second) {
					p3 = l2.second;
					p4 = l2.first;
				}
				else {
					p3 = l2.first;
					p4 = l2.second;
				}
				//p2 -= p1;
				//p4 -= p3;
				return atan2(p1.y, p1.x) < atan2(p3.y, p3.x);
				//return atan2(p2.y, p2.x) < atan2(p4.y, p4.x);
			});
		}
	}

	vector<vector<pair<int, bool> > > pieces;
	vector<bool> usedLine(paths.size() * 2, false);
	for (int i = 0; i < ptToPath.size(); ++i) {
		auto & neighbors = ptToPath[i];
		for (int j = 0; j < neighbors.size(); ++j) {
			auto neigh = neighbors[j];
			if (!usedLine[neigh.first * 2 + neigh.second]) {
				//make piece
				vector<pair<int, bool> > piece;

				auto first = neigh;
				auto curr = first;
				do {
					piece.push_back(curr);
					usedLine[curr.first * 2 + curr.second] = true;
					//get next
					int nextP = endPts[curr.first].second;
					if (curr.second) {
						nextP = endPts[curr.first].first;
					}
					auto & nextNeighbors = ptToPath[nextP];
					bool foundNext = false;
					for (int k = 0; k < nextNeighbors.size(); ++k) {
						if (nextNeighbors[k].first == curr.first) {
							curr = nextNeighbors[(k + 1) % nextNeighbors.size()];
							foundNext = true;
							break;
						}
					}
					if (!foundNext) {
						cout << "next not found" << endl;
						break;
					}
					if (piece.size() > 1000) {
						cout << "infinite piece" << endl;
						break;
					}
				} while (curr != first);
				//if (piece.size() > 2) {
					pieces.push_back(piece);
				//}
			}
		}
	}
	vector<bool> isBoundary(pieces.size(), false);
	for (int i = 0; i < pieces.size(); ++i) {
		float area = 0;
		auto & piece = pieces[i];
		for (int j = 0; j < piece.size(); ++j) {
			auto & line = endPts[piece[j].first];
			auto path = paths[piece[j].first];
			//auto p1 = pts[line.first];
			//auto p2 = pts[line.second];
			auto verts = path.getOutline()[0];
			if (piece[j].second) {
				reverse(verts.begin(), verts.end());
			}
			for (int k = 0; k < verts.size() - 1; ++k) {
				auto p1 = verts[k];
				auto p2 = verts[k + 1];
				float a = -p1.x*p2.y + p1.y*p2.x;

				//if (piece[j].second) a *= -1;
				area += a;
			}

			//float a = -p1.x*p2.y + p1.y*p2.x;
			//if (piece[j].second) a *= -1;
			//area += a;
		}
		//cout << area << endl;
		if (area < 0) {
			isBoundary[i] = true;
			cout << "boundary " << i << " " << piece.size() << endl;
		}
	}
	vector<int> lineToPiece(paths.size() * 2, -1);
	for (int i = 0; i<pieces.size(); ++i) {
		auto & piece = pieces[i];
		for (int j = 0; j < piece.size(); ++j) {
			auto & l = piece[j];
			lineToPiece[l.first * 2 + l.second] = i;
		}
	}

	vector<bool> marked(pieces.size(), false);
	vector<bool> markedLines(paths.size(), false);
	int firstPiece = 0;
	//get first piece
	float minD = 9e9;
	ofVec3f center(svg.getWidth() / 2, svg.getHeight() / 2);
	vector<ofVec3f> centers(pieces.size());
	for (int i = 0; i < pieces.size(); ++i) {
		auto & piece = pieces[i];
		if (!isBoundary[i]) {
			ofVec3f pt(0, 0, 0);
			for (int j = 0; j < piece.size(); ++j) {
				auto & li = endPts[piece[j].first];
				pt += pts[li.first];
				pt += pts[li.second];
			}
			pt /= piece.size() * 2;
			float d = pt.distanceSquared(center);
			if (d < minD) {
				minD = d;
				firstPiece = i;
			}
			centers[i] = pt;
		}
	}
	list<pair<int, int>> pieceStack;
	pieceStack.push_back(make_pair(firstPiece, 0));
	marked[firstPiece] = true;
	vector<ofPath> newPaths;
	vector<bool> status(pieces.size(), false);
	priority_queue < pair<float, int>, vector<pair<float, int> >, greater<pair<float, int> > > pieceQueue;
	ofBeginSaveScreenAsPDF(filename.substr(0, filename.size() - 4) + "_ordered.pdf", false, false, ofRectangle(0, 0, svg.getWidth()+1, svg.getHeight()+1));
	
	pieceQueue.push(make_pair(0, firstPiece));
	while (pieceQueue.size() > 0) {
		auto pp = pieceQueue.top();
		pieceQueue.pop();
		float d = pp.first;
		int p = pp.second;
		auto & center1 = centers[p];
		if (!status[p]) {
			status[p] = true;

			auto & piece = pieces[p];
			for (int i = 0; i < piece.size(); ++i) {
				auto l = piece[i];
				int neighbor = lineToPiece[l.first * 2 + (1 - l.second)];
				if (neighbor != -1 && !status[neighbor]) {
					if (!isBoundary[neighbor]) {
						auto & center2 = centers[neighbor];
						pieceQueue.push(make_pair(d + (center1 - center2).length(), neighbor));
					}
				}
				if (!markedLines[l.first]) {
					if (!isBoundary[neighbor]) {
						markedLines[l.first] = true;
						auto & li = paths[l.first];
						li.draw();
						newPaths.push_back(li);
					}
				}
			}

		}
	}
	//draw boundary last
	for (int i = 0; i < pieces.size(); ++i) {
		if (isBoundary[i]) {
			auto & piece = pieces[i];
			for (int j = 0; j < piece.size(); ++j) {
				auto & l = piece[j];
				newPaths.push_back(paths[l.first]);
				paths[l.first].getOutline()[0].draw();
			}
		}
	}
	
	ofEndSaveScreenAsPDF();
	
	/*
	ofVec2f center(svg.getWidth() / 2, svg.getHeight() / 2);
	float minD = 9e9;
	int minI = 0;
	for (int i = 0; i < paths.size(); ++i) {
		if (!merged[i]) {
			auto & ends = endPts[i];

			ofVec2f p1 = pts[ends.first];
			ofVec2f p2 = pts[ends.first];
			ofVec2f mid = (p1 + p2)*0.5;
			float d = mid.distanceSquared(center);
			if (d < minD) {
				minD = d;
				minI = i;
			}
		}
	}

	ofBeginSaveScreenAsPDF(filename.substr(0,filename.size()-4)+"_ordered.pdf",false,false, ofRectangle(0,0,svg.getWidth(),svg.getHeight()));

	vector<bool> marked(paths.size(), false);
	list<int> lineStack;
	lineStack.push_back(minI);
	marked[minI] = true;
	vector<ofPath> newPaths;
	while (lineStack.size()>0) {
		int li = lineStack.front();
		lineStack.pop_front();
		paths[li].setStrokeWidth(1);
		paths[li].draw();
		newPaths.push_back(paths[li]);

		int p1 = endPts[li].first;
		int p2 = endPts[li].second;
		for (int i = 0; i < ptToPath[p1].size(); ++i) {
			int li2 = ptToPath[p1][i];
			if (!marked[li2]) {
				marked[li2] = true;
				lineStack.push_back(li2);
			}
		}
		for (int i = 0; i < ptToPath[p2].size(); ++i) {
			int li2 = ptToPath[p2][i];
			if (!marked[li2]) {
				marked[li2] = true;
				lineStack.push_back(li2);
			}
		}
	}
	ofEndSaveScreenAsPDF();
	*/

	
	paths = newPaths;

}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(255);
	animI = min(animI, (int)paths.size()-1);
	ofSetColor(255, 0, 0);
	
	for (int i = 0; i <= animI; ++i) {
		paths[i].draw();
	}
	if (ofGetFrameNum() % 3 == 0) {
		if (paths.size() > 0)animI = (animI + 1) % paths.size();
	}
	/*
	for (auto & path : paths) {
		//path.draw();
	}
	for (int i = 0; i < pts.size(); ++i) {
		ofSetColor(0);
		if (i == selectedPt) {
			ofSetColor(255,0,255);
		}
		ofCircle(pts[i], 2);
	}
	ofColor colors[5];
	colors[0] = ofColor(255, 0, 0);
	colors[1] = ofColor(0, 255, 0);
	colors[2] = ofColor(0, 0, 255);
	colors[3] = ofColor(255, 0,255);
	colors[4] = ofColor(0, 255, 255);
	
	
	ofSetColor(0, 0, 255);
	ofSetColor(255);
	for (int i = 0; i < pts.size(); ++i) {
		auto neighbors = ptToPath[i];
		ofPushMatrix();
		if (neighbors.size() == 1) {
			ofTranslate(1, 0);
			for (auto n : neighbors) {
				paths[n.first].setColor(ofColor(255,0,0));
				paths[n.first].draw();
			}
		}
		else if (neighbors.size() == 2) {
			ofTranslate(-1, 0);
			for (auto n : neighbors) {
				paths[n.first].setColor(ofColor(0, 255, 0));
				paths[n.first].draw();
			}
		}
		else if (neighbors.size() == 3) {
			ofTranslate(0, 1);
			for (auto n : neighbors) {
				paths[n.first].setColor(ofColor(0, 0, 255));
				paths[n.first].draw();
			}
		}
		else if (neighbors.size() == 4) {
			ofTranslate(0, -1);
			for (auto n : neighbors) {
				paths[n.first].setColor(ofColor(255, 0, 255));
				paths[n.first].draw();
			}
		}
		ofPopMatrix();
		
	}
	if (pts.size() > 0) {
		auto neighbors = ptToPath[selectedPt];
		for (int i = 0; i < neighbors.size(); ++i) {
			auto path = paths[neighbors[i].first];
			path.setColor(colors[i]);
			path.draw();
		}
	}
	*/
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	int closest = 0;
	float minD = 9e9;
	for (int i = 0; i < pts.size(); ++i) {
		float d = pts[i].distanceSquared(ofPoint(x, y));
		if (d < minD) {
			closest = i;
			minD = d;
		}
	}
	selectedPt = closest;
	if (pts.size() > 0) {
		auto neighbors = ptToPath[selectedPt];
		for (int i = 0; i < neighbors.size(); ++i) {
			auto path = paths[neighbors[i].first];
			vector<ofPolyline> outline = path.getOutline();
			//assume only one line per path
			
			ofVec2f startPt = outline[0].getVertices().front();
			ofVec2f endPt = outline[0].getVertices().back();


			ofVec2f startTan = outline[0].getVertices()[2] - startPt;
			ofVec2f endTan = outline[0].getVertices()[outline[0].getVertices().size() - 3] - endPt;
			startTan.normalize();
			endTan.normalize();
			cout << startTan << " " << endTan << endl;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
	order(dragInfo.files[0]);
}
