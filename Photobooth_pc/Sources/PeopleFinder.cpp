// -----------------------------------------------------------------------------
//  Copyright (c) 2017-2020, IXBLUE.
// -----------------------------------------------------------------------------
//  N O T I C E
//
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE.
// -----------------------------------------------------------------------------

#include "PeopleFinder.h"
#include "Model.h"
#include "Tensor.h"
#include "QFile"
#include "QDebug"

using namespace cv;
using namespace std;

PeopleFinder::PeopleFinder(const QString& filepath, float gpuReq)
{
	if(QFile::exists(filepath))
	{
		_pModel = new Model(filepath, gpuReq);	

		size_t pos = 0;
		TF_Status* status = TF_NewStatus();

		TF_Operation* oper = TF_GraphNextOperation(_pModel->graph, &pos);
		qDebug() << "Input layer : " << TF_OperationName(oper);
		TF_Output op = {oper, 0};

		// Get input shape
		int nInputDims = TF_GraphGetTensorNumDims(_pModel->graph, op, status);
		auto *inputDims = new int64_t[nInputDims];
		TF_GraphGetTensorShape(_pModel->graph, op, inputDims, nInputDims, status);
		_imgW = (int)inputDims[1];
		_imgH = (int)inputDims[2];
		_nbOfChannels = inputDims[3];
		delete[] inputDims;

		qDebug() << "coucou";

		TF_Operation* test1 = TF_GraphOperationByName(_pModel->graph, "ImageTensor");
		TF_Operation* test2 = TF_GraphOperationByName(_pModel->graph, "SemanticPredictions");

		_pInput = new Tensor(*_pModel, {test1, 0});
		_pOutput = new Tensor(*_pModel, { test2, 0 });
		_nbOfClasses = _pOutput->shape[3];

		_batchSize = 4;
		_inputs.resize(_batchSize * _imgH * _imgW * 3);

		_pOutput->shape[0] = _batchSize;
		_pOutput->shape[1] = _imgW;
		_pOutput->shape[2] = _imgH;

		// Init model
		findHorizon(cv::Mat(_imgW, _imgH, CV_8UC3, Scalar(0)));
	}
	else
	{
		qDebug() << QString("Weights not found at : %s").arg(filepath);
	}
}

PeopleFinder::~PeopleFinder()
{
	delete _pModel;
	delete _pInput;
	delete _pOutput;
}

Mat PeopleFinder::findHorizon(const Mat& img)
{
	vector<Mat> v;
	v.push_back(img);
	return findHorizon(v)[0];
}
		
vector<Mat> PeopleFinder::findHorizon(const vector<Mat>& images)
{
	vector<Mat> out;

	if (_pModel != nullptr)
	{
		int nBatches = float(images.size()) / _batchSize + 0.99;
		for (int b = 0; b < nBatches; b++)
		{
			// Set input data
			Mat rsz, norm;
			int inSize = _imgH * _imgW * 3;
			for (int i = 0; b * _batchSize + i < images.size() && i < _batchSize; i++)
			{
				Mat img = images[b * _batchSize + i];
				cv::Mat prepImg = prepImage(img);
				memcpy(_inputs.data() + inSize * i, prepImg.data, inSize);
			}

			// Run model on batch
			vector<float> output;
			if (_pModel != nullptr)
			{
				_pInput->set_data(_inputs);												// Feed data to input tensor
				_pModel->run(_pInput, _pOutput);										// Run inference
				output = _pOutput->get_data<float>();									// Read the probabilities of the output
			}

			// Extract results
			Mat mask(_imgH, _imgW, CV_32FC1, Scalar(0));
			int stride = _imgH * _imgW * _pOutput->shape[3];
			for (int i = 0; b * _batchSize + i < images.size() && i < _batchSize; i++)
			{
				//TODO : extract results

				const Mat& img = images[b * _batchSize + i];
				Mat finalMask;
				resize(mask, finalMask, Size(img.cols, img.rows));						// Resize to original size
				out.push_back(finalMask);												// Add to outputs
			}
		}
	}
	else
	{
		qDebug() << "Could not find horizon : model is not loaded";
	}	

	return out;
}

cv::Mat PeopleFinder::prepImage(const cv::Mat& image)
{
	return image;
}
