// C++
#include <iostream>
// OpenCV
#include <opencv.hpp>
// Self
#include "src/core/capture.h"

cv::VideoCapture capture;
cv::TickMeter tm;
cv::Mat capt;

/*
 * ����ͷ��ʼ��
 * @author ���ս�
 * @date 2022/5/30
 */
int Capture::initCapture(int capture_id, bool debug) {
	capture.open(capture_id);
	if (!capture.isOpened()) {
		return 0;
	}
	if (debug) {
		capture.set(cv::CAP_PROP_SETTINGS, 1);			//�������ý���
	}
	capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);		//���� 640
	capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);		//�߶� 480
	//capture.set(cv::CAP_PROP_FPS, 0);					//֡�� 0
	//capture.set(cv::CAP_PROP_BRIGHTNESS, 0);			//���� 0
	//capture.set(cv::CAP_PROP_CONTRAST, 32);			//�Աȶ� 32
	//capture.set(cv::CAP_PROP_SATURATION, 56);			//���Ͷ� 56
	//capture.set(cv::CAP_PROP_HUE, 0);					//ɫ�� 0
	//capture.set(cv::CAP_PROP_EXPOSURE, -6);			//�ع� -6
	//capture.set(cv::CAP_PROP_FRAME_COUNT, -1);		//��֡�� -1
	//capture.set(cv::CAP_PROP_FOCUS, 85);
	return 1;
}

/*
 * ������֤
 * @author ���ս�
 * @date 2022/6/3
 */
int Capture::recognition(int capture_id, bool debug) {
	bool face_on = false;
	bool matchedFace_on = false;
	bool QRcode_on = false;
	int unknown_times = 0;
	int fontFace = cv::FONT_HERSHEY_COMPLEX;
	double fontScale = 0.5;       //�������ű�
	int thickness = 1;
	int baseLine = 0;
	while (char(cv::waitKey(1)) != ' ') {
		tm.start();
		capture >> capt;
		cv::resize(capt, capt, { 640,480 });
		cv::flip(capt, capt, 1);	// ����
		if (debug) {
			cv::imwrite("./debug.jpg", capt);
			//capt = cv::imread("./debug.jpg");
		}

		std::vector<float> mask_result = MaskAlgo::recognize(capt);
		if (mask_result.empty()) {
			// ������Ϊ�գ�˵��û���ҵ�����
			tm.stop();
			putText(capt, cv::format("FPS : %.2f", (float)tm.getFPS()), cv::Point(0, 15), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
			cv::imshow("capt", capt);
			continue;
		}
		int x = std::max((int)mask_result[0], 0);
		int y = std::max((int)mask_result[1], 0);
		int w = std::min((int)mask_result[2], 640 - x);
		int h = std::min((int)mask_result[3], 480 - y);
		float points = mask_result[4];
		bool withMask = !mask_result[5];

		cv::Mat face = capt(cv::Rect(x, y, w, h));
		std::vector<std::shared_ptr<faceInfo>> infoList;
		std::string result;
		cv::Scalar color;

		//if ((face.rows * face.cols) > 20000) {
		if (face.data) {
			resize(face, face, { 200 , 300 });
			//cout << "rows:" << face.rows << ",cols:" << face.cols << endl;
			face_on = true;
			imshow("face", face);
			FaceAlgo::detectFace(face, infoList);
			FaceAlgo::matchFace(face, infoList, true);
		}
		else if (face_on) {
			face_on = false;
			cv::destroyWindow("face");
		}

		if (infoList.empty()) {
			//���û��ʶ����
			tm.stop();
			putText(capt, cv::format("FPS : %.2f", (float)tm.getFPS()), cv::Point(0, 15), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
			cv::imshow("capt", capt);
			infoList.clear();
			continue;
		}

		result.append(infoList[0]->name);
		if (infoList[0]->name == "Unknown") {
			unknown_times++;
			if (matchedFace_on) {
				matchedFace_on = false;
				cv::destroyWindow("matchedFace");
			}
			if (5 == unknown_times) {
				tm.stop();
				//Register::registFaceByQRcode(face);
				Register::registFaceByInput(face);
				unknown_times = 0;
				tm.start();
			}
		}
		else {
			unknown_times = 0;
			matchedFace_on = true;
			cv::imshow("matchedFace", cv::imread("./data/registFace/" + infoList[0]->name + ".jpg"));
		}

		if (result.empty()) {
			result.append("Too_far");
			ResultBack::too_far();
		}

		if (withMask) {
			result.append(" mask(" + std::to_string(points) + ")");
			color = cv::Scalar(0, 205, 0);
		}
		else {
			result.append(" nomask(" + std::to_string(points) + ")");
			color = cv::Scalar(0, 0, 255);
			ResultBack::no_mask();
		}
		cv::Size textSize = cv::getTextSize(result, fontFace, fontScale, thickness, &baseLine);
		rectangle(capt, cv::Rect(x, y, w, h), color, 2, 8, 0);
		//Rect(x - 1, y + h, result.length() * 12, 15);
		rectangle(capt, cv::Rect(x - 1, y + h, textSize.width, textSize.height + 3), color, cv::FILLED);
		putText(capt, result, cv::Point(x, y + h + 10), fontFace, fontScale, cv::Scalar(255, 255, 255), thickness);
		result.clear();
		tm.stop();
		putText(capt, cv::format("FPS : %.2f", (float)tm.getFPS()), cv::Point(0, 15), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
		cv::imshow("capt", capt);
		infoList.clear();
	}
	capture.release();
	cv::destroyAllWindows();
	return 0;
}

/*
 * ������ :)
 * @author ���ս�
 * @date 2022/7/16
 */
cv::Mat Capture::getCapt() {
	cv::Mat capt;
	capture >> capt;
	return capt;
}
