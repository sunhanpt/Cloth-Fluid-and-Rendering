#include <opencv.hpp>
#include <core\core.hpp>
#include <highgui\highgui.hpp>

#include <math.h>

using namespace cv;

const float MAXSATURATION = 10.0;
const int DIMX = 32;
const int DIMY = 32;

const int WIDTHBASE = 8;

float getMax(float x, float y) {
	if (x > y) return x;
	else return y;
}
float getMin(float x, float y) {
	if (x < y) return x;
	else return y;
}

class Maps {
public:
	Maps() {
		scale = 512;
		init();
	}

	Maps(int s) {
		scale = s;
		init();
	}
	Maps(int s, std::string str) {
		scale = s;
		name = str;

		init();
		readBmap();
		readMSmap();
	}

	void setScale(int s) {
		scale = s;
		saturationSize = s * s / 64;
	}
	int getScale() {
		return scale;
	}
	void setLineWidth(int l) {
		lineWidth = l;
	}
	int getLineWidth() {
		return lineWidth;
	}
	void setName(std::string str) {
		name = str;

		init();
		readBmap();
		readMSmap();
	}
	std::string getName() {
		return name;
	}
	void setMaxmap(bool mm) {
		maxMap = mm;
		if (maxMap) {
			readMSmap();
		}
	}


	void writeToFile() {
		renewSaturationMap();
		imwrite("../../data/textures/s" + name + ".bmp", smat);
		//std::cout << "file" << std::endl;
	}


	void renewAbsorbing(int x, int y, Vec2 pos, float s) {

		Absorbing0(x, y, pos, s);


	}
	/*for test*/
	void renewAbsorbing(int x, int y, Vec2 pos, float s, int level) {
		int sx = x * 2 + int(pos.x) - 1;
		int sy = y * 2 + int(pos.y) - 1;
		if (sx < 0) sx = 0;
		if (sx >= rows) sx = rows - 1;
		if (sy < 0) sy = 0;
		if (sy >= cols) sy = cols - 1;

		int t = sx * cols + sy;
		if (level == 0) {
			saturationRow[t] += s;
		}
		else if (level == 1) {
			saturationCol[t] += s;
		}
	}
	void renewPointTheta(int i, Vec4 thetas) {
		pointThetas[i] = thetas;
	}
	void renewDiffusing(float kDiffusion, float kDiffusionGravity) {
		Diffusing(kDiffusion, kDiffusionGravity );
	}

private:
	Mat smat, bmat;

	std::vector<float> saturationRow, saturationCol;
	std::vector<int> bitMap;
	std::vector<float> msMap;

	std::vector<bool> row, col;

	std::vector<Vec4> pointThetas;

	int scale;
	int rows, cols;
	int saturationSize;
	int lineWidth = 1;
	std::string name;

	bool maxMap;

	float kDiffusionSide, kDiffusionLevel, kDiffustionLevelGravity;

	void init() {
		smat = Mat(scale, scale, CV_8UC1);
		initiateSaturation();
		row.resize(rows, false);
		col.resize(cols, false);
		kDiffusionSide = 0.1;
		kDiffusionLevel = 0.05;
		kDiffustionLevelGravity = 0.2;
		pointThetas.resize(DIMX * DIMY);

		maxMap = false;
	}

	void initiateSaturation() {
		rows = (DIMX - 1) * 2; 
		cols = (DIMY - 1) * 2;
		saturationSize = rows * cols;
		saturationRow.resize(saturationSize, 0);
		saturationCol.resize(saturationSize, 0);
		
		for (int i = 0; i < smat.rows; i++) {
			for (int j = 0; j < smat.cols; j++) {
				smat.at<uchar>(i, j) = 0;
			}
		}

		//imwrite("../../data/textures/s" + name + ".bmp", smat);
	}

	void renewSaturationMap() {
		int idx = 0;
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				float t = 0;
				if (bitMap[idx] == 0) {
					t = saturationRow[idx];
				}
				else if (bitMap[idx] == 255) {
					t = saturationCol[idx];
				}
				idx++;
				float tmp = t / MAXSATURATION * 255;
				if (tmp > 255) tmp = 255;
				if (t > 0 && (int)tmp == 0) tmp = 1;

				for (int ii = 0; ii < 8; ii++) {
					for (int jj = 0; jj < 8; jj++) {
						smat.at<uchar>(i * 8 + ii, j * 8 + jj) = tmp;
					}
				}
			}
		}
	}

	void readBmap() {
		bmat = imread("../../data/textures/brc" + name + ".bmp");

		bitMap.resize(saturationSize, 0);
		int idx = 0;
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				cv::Vec3b &bgr = bmat.at<cv::Vec3b>(i * 8 + 4, j * 8 + 4);
				if (bgr[0] == 255) row[i] = true;
				if (bgr[1] == 255) col[j] = true;
				bitMap[idx++] = (int)(bgr[2]);
			}
		}
	}

	void readMSmap() {
		if (msMap.size() > 0) return;

		Mat msmat = imread("../../data/textures/ms" + name + ".bmp");
		if (msmat.rows == 0) {
			maxMap = false;
			return;
		}

		msMap.resize(saturationSize, 0);
		int idx = 0;
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				cv::Vec3b &bgr = msmat.at<cv::Vec3b>(i, j);
				//msMap[idx++] = (float)bgr[0] / 255 * MAXSATURATION;
				float tmp = 1.0 - (float)bgr[0] / 255;
				tmp = 1 - pow(tmp, 1.0 / 6);
				msMap[idx++] = tmp * MAXSATURATION;
			}
		}
	}

	void renewSaturation(int x, int y, Vec2 pos, float s) {

		int px = (x * 2 + int(pos.x)) * 8;
		int py = (y * 2 + int(pos.y)) * 8;

		int t = smat.at<uchar>(px, py);
		t += (int)(s / MAXSATURATION * 255);
		if (t > 255) t = 255;

		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				smat.at<uchar>(px + i, py + j) = t;
			}
		}
	}

	void Absorbing0(int x, int y, Vec2 pos, float s) {

		int sx = x * 2 + int(pos.x) - 1;
		int sy = y * 2 + int(pos.y) - 1;
		//std::cout << x << ' ' << y << ' ' << sx << ' ' << sy << ' ';
		if (sx < 0) sx = 0;
		if (sx >= rows) sx = rows - 1;
		if (sy < 0) sy = 0;
		if (sy >= cols) sy = cols - 1;

		int t = sx * cols + sy;
		//std::cout << mx << ' ' << my << std::endl;
		//std::cout << bmat.channels() << std::endl;
		//std::cout << t << std::endl;
		float max = MAXSATURATION;
		if (maxMap && msMap[t] < MAXSATURATION) {
			max = msMap[t];
			//max = 1.0;
		}

		if (bitMap[t] == 0) {
			saturationRow[t] += s;
			if (saturationRow[t] > max) saturationRow[t] = max;
			//std::cout << s << std::endl;
			//std::cout << saturationRow[sx * DIMX * 2 + sy] << std::endl;
		}
		else if (bitMap[t] == 255) {
			saturationCol[t] += s;
			if (saturationCol[t] > max) saturationCol[t] = max;
			//std::cout << saturationCol[sx * DIMX * 2 + sy] << std::endl;
		}
		else {
			//std::cout << "no\n";
		}
	}

	void Diffusing(float kDiffusion, float kDiffusionGravity) {
		std::vector<float> deltasRow;
		std::vector<float> deltasCol;
		deltasRow.resize(saturationSize, 0);
		deltasCol.resize(saturationSize, 0);

		for (int i = 0; i < saturationSize; i++) {
			int x = i / cols;
			int y = i % cols;
			if (!row[x]) continue;

			float sum = 0;
			float si = saturationRow[i];
			float ds1 = 0, ds2 = 0, ds3 = 0, ds4 = 0, ds5 = 0;

			Vec4 thetas = pointThetas[(x + 1) / 2 * DIMY + (y + 1) / 2];
			float theta = 0;
			if (bitMap[i] == 0){
				theta = 1;
			}

			/*TODO: solve problem: next is empty*/
			////without gravity
			////row self
			//if (y > 0) {
			//	ds1 = getMax(0.0, kDiffusion * (si - saturationRow[i - 1]));
			//	sum += ds1;
			//}
			//if (y < cols - 1) {
			//	ds2 = getMax(0.0, kDiffusion * (si - saturationRow[i + 1]));
			//	sum += ds2;
			//}
			////row side
			//if (x > 0 && row[x - 1]) {
			//	ds3 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationRow[i - cols])));
			//	sum += ds3;
			//}
			//if (x < rows - 1 && row[x + 1]) {
			//	ds4 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationRow[i + cols])));
			//	sum += ds4;
			//}
			////row level
			//if (col[y]) {
			//	ds5 = getMax(0.0, kDiffusionLevel * (kDiffusion * (si - saturationCol[i])));
			//	sum += ds5;
			//}

			//with gravity
			//row self
			if (y > 0) {
				ds1 = getMax(0.0, kDiffusion * (si - saturationRow[i - 1]) + kDiffusionGravity * si * thetas[0]);
				sum += ds1;
			}
			if (y < cols - 1) {
				ds2 = getMax(0.0, kDiffusion * (si - saturationRow[i + 1]) + kDiffusionGravity * si * thetas[1]);
				sum += ds2;
			}
			//row side
			if (x > 0 && row[x - 1]) {
				ds3 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationRow[i - cols]) + kDiffusionGravity * si * thetas[2]));
				sum += ds3;
			}
			if (x < rows - 1 && row[x + 1]) {
				ds4 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationRow[i + cols]) + kDiffusionGravity * si * thetas[3]));
				sum += ds4;
			}
			//row level
			if (col[y]) {
				ds5 = getMax(0.0, kDiffusionLevel * (kDiffusion * (si - saturationCol[i]) + kDiffustionLevelGravity * si * theta));
				sum += ds5;
			}

			float normFac = 1.0;
			if (sum > si) {
				normFac = si / sum;
			}

			if (y > 0) {
				deltasRow[i] += -normFac * ds1;
				deltasRow[i - 1] += normFac * ds1;
			}
			if (y < cols - 1) {
				deltasRow[i] += -normFac * ds2;
				deltasRow[i + 1] += normFac * ds2;
			}
			if (x > 0 && row[x - 1]) {
				deltasRow[i] += -normFac * ds3;
				deltasRow[i - cols] += normFac * ds3;
			}
			if (x < rows - 1 && row[x + 1]) {
				deltasRow[i] += -normFac * ds4;
				deltasRow[i + cols] += normFac * ds4;
			}
			if (col[y]) {
				deltasRow[i] += -normFac * ds5;
				deltasCol[i] += normFac * ds5;
			}
		}


		for (int i = 0; i < saturationSize; i++) {
			int x = i / cols;
			int y = i % cols;
			if (!col[y]) continue;

			float sum = 0;
			float si = saturationCol[i];
			float ds1 = 0, ds2 = 0, ds3 = 0, ds4 = 0, ds5 = 0;

			Vec4 thetas = pointThetas[(x + 1) / 2 * DIMY + (y + 1) / 2];		
			//if (x == 61 && y == 0) {
			//	std::cout << "thetas: " << thetas[0] << ' ' << thetas[1] << ' ' << thetas[2] << ' ' << thetas[3] << std::endl;
			//}

			float theta = 0;
			if (bitMap[i] == 255){
				theta = 1;
			}

			/*TODO: solve problem: next is empty*/
			////without gravity
			////col self
			//if (x > 0) {
			//	ds1 = getMax(0.0, kDiffusion * (si - saturationCol[i - cols]));
			//	sum += ds1;
			//}
			//if (x < rows - 1) {
			//	ds2 = getMax(0.0, kDiffusion * (si - saturationCol[i + cols]));
			//	sum += ds2;
			//}
			////col side
			//if (y > 0 && col[y - 1]) {
			//	ds3 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationCol[i - 1])));
			//	sum += ds3;
			//}
			//if (y < cols - 1 && col[y + 1]) {
			//	ds4 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationCol[i + 1])));
			//	sum += ds4;
			//}
			////col level
			//if (row[x]) {
			//	ds5 = getMax(0.0, kDiffusionLevel * (kDiffusion * (si - saturationRow[i])));
			//	sum += ds5;
			//}

			//with gravity
			//col self
			if (x > 0) {
				ds1 = getMax(0.0, kDiffusion * (si - saturationCol[i - cols]) + kDiffusionGravity * si * thetas[2]);
				sum += ds1;
			}
			if (x < rows - 1) {
				ds2 = getMax(0.0, kDiffusion * (si - saturationCol[i + cols]) + kDiffusionGravity * si * thetas[3]);
				sum += ds2;
			}
			//col side
			if (y > 0 && col[y - 1]) {
				ds3 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationCol[i - 1]) + kDiffusionGravity * si * thetas[0]));
				sum += ds3;
			}
			if (y < cols - 1 && col[y + 1]) {
				ds4 = getMax(0.0, kDiffusionSide * (kDiffusion * (si - saturationCol[i + 1]) + kDiffusionGravity * si * thetas[1]));
				sum += ds4;
			}
			//col level
			if (row[x]) {
				ds5 = getMax(0.0, kDiffusionLevel * (kDiffusion * (si - saturationRow[i]) + kDiffustionLevelGravity * si * theta));
				sum += ds5;
			}

			float normFac = 1.0;

			if (sum > si) {
				normFac = si / sum;
			}

			if (x > 0) {
				deltasCol[i] += -normFac * ds1;
				deltasCol[i - cols] += normFac * ds1;
			}
			if (x < rows - 1) {
				deltasCol[i] += -normFac * ds2;
				deltasCol[i + cols] += normFac * ds2;
			}
			if (y > 0 && col[y - 1]) {
				deltasCol[i] += -normFac * ds3;
				deltasCol[i - 1] += normFac * ds3;
			}
			if (y < cols - 1 && col[y + 1]) {
				deltasCol[i] += -normFac * ds4;
				deltasCol[i + 1] += normFac * ds4;
			}
			if (row[x]) {
				deltasCol[i] += -normFac * ds5;
				deltasRow[i] += normFac * ds5;
			}
		}


		//renew
		for (int i = 0; i < saturationSize; i++) {
			float max = MAXSATURATION;
			if (maxMap && msMap[i] < MAXSATURATION) {
				max = msMap[i];
			}

			saturationRow[i] += deltasRow[i];

			if (saturationRow[i] < 0) {
				saturationRow[i] = 0;
			}
			if (saturationRow[i] > max) {
				saturationRow[i] = max;
			}

			saturationCol[i] += deltasCol[i];
			if (saturationCol[i] < 0) {
				saturationCol[i] = 0;
			}
			if (saturationCol[i] > max) {
				saturationCol[i] = max;
			}
		}

	}

};