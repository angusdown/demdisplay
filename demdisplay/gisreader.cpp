#include <gisreader.h>
#include <iostream>
#define STEP 16
void transformToUtm(double& x, double& y, OGRSpatialReference* Original)
{
	OGRSpatialReference    oUTM, *poLatLong;
	OGRCoordinateTransformation *poTransform;

	oUTM.importFromEPSG(26911);
	poLatLong = oUTM.CloneGeogCS();

	poTransform = OGRCreateCoordinateTransformation(Original, &oUTM);
	if (poTransform == NULL)
	{
		return;
	}

	if (!poTransform->Transform(1, &x, &y))
	{
		cout << "FAIL" << endl;
	}
};

void ComputeGeoProperties(GDALDataset *poDataset, int width, int height, double& x, double& y, double& xright, double& ybottom, double& xres, double& yres)
{
	string proj;
	proj = string(poDataset->GetProjectionRef());

	OGRSpatialReference sr2, destSRS;
	OGRCoordinateTransformation* poTransform2=nullptr;
	char* test = &proj[0];
	sr2.importFromWkt(&test);
	int nFlag;
	double adfGeoTransform[6];

	if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None)
	{
		printf("Origin = (%.6f,%.6f)\n", adfGeoTransform[0], adfGeoTransform[3]);
		printf("Pixel Size = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5]);
		x = adfGeoTransform[0];
		y = adfGeoTransform[3];
		//if (sr2.IsProjected())
		//{
		//	destSRS.importFromEPSG(4326);
		//	poTransform2 = OGRCreateCoordinateTransformation(&sr2, &destSRS);
		//	nFlag = poTransform2->Transform(1, &x, &y, nullptr);
		//	adfGeoTransform[1]=adfGeoTransform[1]/111000;
		//	adfGeoTransform[5]=adfGeoTransform[5]/111000;
		//}

		xright = x + adfGeoTransform[1] * (double)(width);
		ybottom = y + adfGeoTransform[5] * (double)(height);
	}
	else
	{
		return;
	}

	//if (!nFlag)
	//{
	//	OGRCoordinateTransformation::DestroyCT(poTransform2);
	//}
	//OGRCoordinateTransformation::DestroyCT(poTransform2);

	double x2 = 0;
	double absoluteW = xright - x;
	double absoluteH = y - ybottom;

	xres = absoluteW / (width);
	yres = absoluteH / (height);
};



bool getRawValuesFromFile(string fname, vector<vector<float>>& vecs, float& min, float& max, float& xres, float& yres, string& projection, double& XORIGIN, double& YORIGIN, int& W, int& H)
{
	GDALDataset *poDataset;
	GDALAllRegister();
	poDataset = (GDALDataset*)GDALOpen(fname.c_str(), GA_ReadOnly);
	if (poDataset == NULL)
	{
		cout << "Failure to load file due to not existing or write permissions!!!" << endl;
		return false;
	}

	OGRSpatialReference    oUTM;

	oUTM.SetProjCS("UTM 11 / WGS84");
	oUTM.SetWellKnownGeogCS("WGS84");
	oUTM.SetUTM(11);

	projection = string(poDataset->GetProjectionRef());
	cout<<projection<<endl;
	cout << "Data size: " << GDALGetRasterXSize(poDataset) << " " << GDALGetRasterYSize(poDataset) << endl;

	GDALRasterBand  *poBand;
	int             nBlockXSize, nBlockYSize;
	int             bGotMin, bGotMax;
	double          adfMinMax[2];
	poBand = poDataset->GetRasterBand(1);
	poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);

	printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
		nBlockXSize, nBlockYSize,
		GDALGetDataTypeName(poBand->GetRasterDataType()),
		GDALGetColorInterpretationName(
			poBand->GetColorInterpretation()));
	poBand->ComputeStatistics(TRUE, &adfMinMax[0], &adfMinMax[1], nullptr, nullptr, nullptr,nullptr);
	min = adfMinMax[0]; //= poBand->GetMinimum(&bGotMin);
	max = adfMinMax[1]; //= poBand->GetMaximum(&bGotMax);
	std::cout << "Before allocation " << "Min: " << min << " Max: " << max << endl;

	//if (!(bGotMin && bGotMax))
	//	poBand->ComputeRasterMinMax(FALSE, &adfMinMax[0]);
	int width = poBand->GetXSize();
	int height = poBand->GetYSize();
	double x, y, xright, ybottom, XRES, YRES;

	ComputeGeoProperties(poDataset, width, height, x, y, xright, ybottom, XRES, YRES);
	cout<<"after trans:xres="<<XRES<<";yres="<<YRES<<endl;
	cout<<"after trans:xorigin="<<x<<";yorigin="<<y<<endl;
	float *pafScanline;
	pafScanline = (float *)CPLMalloc(sizeof(float) * width * height);
	auto err = poBand->RasterIO(GF_Read, 0, 0, width, height, pafScanline, width, height, GDT_Float32, 0, 0);
	cout << "Loaded data with status " << err << endl;

	vector<vector<float>> out = vector<vector<float>>(width, vector<float>(height, 0));
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			if (pafScanline[(i)*width + j] > 0)
			{
				out[j][i] = pafScanline[(i)* width + j];
			}
			else
				out[j][i] = 0;
		}
	}
	xres = XRES;
	yres = YRES;
	XORIGIN = x;
	YORIGIN = y;
	CPLFree(pafScanline);
	cout << "Done Loading" << endl;
	vecs = out;
	return true;
}

vec3 ComputeNormal(vec3 center, int i, int j, int width, int height, vector<vector<float>>& data, float Max, float xres, float yres )
{
  vec3 left;
  vec3 right;
  vec3 up;
  vec3 down;
  vec3 sum = vec3(0, 0, 0);
  bool l = false;
  bool r = false;
  bool u = false;
  bool d = false;

  int count = 0;
  if (i - 1 >= 0)
  {
    left = vec3((i - 1) * xres, data[i - 1][j], j * yres);
    left = center - left;
    l = true;
  }
  if (i + 1 < width)
  {
    right = vec3((i + 1) * xres, data[i + 1][j], j * yres);
    right = center - right;
    r = true;
  }
  if (j - 1 >= 0)
  {
    up = vec3((i) * xres, data[i][j - 1], (j - 1) * yres);
    up = center - up;
    u = true;
  }
  if (j + 1 < height)
  {
    down = vec3((i) * xres, data[i][j + 1], (j + 1) * yres);
    down = center - down;
    d = true;
  }

  if (u  && r)
  {
    vec3 v1 = cross(up, right);
    if (v1.y < 0)
    {
      v1 *= -1;
    }
    sum += v1;
    count = count + 1;
  }
  if (u && l)
  {
    vec3 v1 = cross(up, left);
    if (v1.y < 0)
    {
      v1 *= -1;
    }
    sum += v1;
    count = count + 1;
  }
  if (d && r)
  {
    vec3 v1 = cross(down, right);
    if (v1.y < 0)
    {
      v1 *= -1;
    }
    sum += v1;
    count = count + 1;
  }
  if (d && l)
  {
    vec3 v1 = cross(down, left);
    if (v1.y < 0)
    {
      v1 *= -1;
    }
    sum += v1;
    count = count + 1;
  }
  sum /= count;
  auto t = normalize(sum);
  return normalize(sum);
};

void createMesh(vector<vector<float>>& input, float xres, float yres, float max, vector<int>& indices, vector<Vertex>& vertexes)
{
	std::vector<Vertex> vectors = vector<Vertex>();
	vector<int> indexs = vector<int>();

	for (int i = 0; i < input.size() - 2; i++)
	{
		for (int j = 0; j < input[i].size() - 2; j++)
		{
			float UL = (float)(input[i][j]) / (float)(max); // Upper left
			float LL = (float)(input[i + 1][j]) / (float)(max); // Lower left
			float UR = (float)(input[i][j + 1]) / (float)(max); // Upper right
			float LR = (float)(input[i + 1][j + 1]) / (float)(max); // Lower right

			if (UL <= 0.0)
			{
				UL = 0.0;
			}
			if (UR <= 0)
			{
				UR = 0;
			}
			if (LR <= 0)
			{
				LR = 0;
			}
			if (LL <= 0)
			{
				LL = 0;
			}

			vec3 ULV = { i * xres, UL * max, j * yres };
			vec3 LLV = { (i + 1)*xres, LL * max, j * yres };
			vec3 URV = { i * xres, UR * max, (j + 1)*yres };
			vec3 LRV = { (i + 1)*xres, LR * max, (j + 1)*yres };

			// compute smoothed normal
			vec3 a = ComputeNormal(ULV, i, j, input.size(), input[i].size(), input, max, xres, yres);
			vec3 b = ComputeNormal(LLV, i + 1, j, input.size(), input[i].size(), input, max, xres, yres);
			vec3 c = ComputeNormal(URV, i, j + 1, input.size(), input[i].size(), input, max, xres, yres);
			vec3 d = ComputeNormal(LRV, i + 1, j + 1, input.size(), input[i].size(), input, max, xres, yres);

			vectors.push_back(Vertex{ {i * xres, UL, j * yres}, a, {(float)i / (float)input.size(), (float)j / (float)input[i].size()} });
			vectors.push_back(Vertex{ {(i + 1)*xres, LL, j * yres}, b, {(float)(i + 1) / (float)input.size(), (float)j / (float)input[i].size()} });
			vectors.push_back(Vertex{ {i * xres, UR, (j + 1)*yres}, c, {(float)i / (float)input.size(), (float)(j + 1) / (float)input[i].size()} });
			vectors.push_back(Vertex{ {(i + 1)*xres, LR, (j + 1)*yres}, d, {(float)(i + 1) / (float)input.size(), (float)(j + 1) / (float)input[i].size()} });
			indexs.push_back(vectors.size() - 4);
			indexs.push_back(vectors.size() - 1);
			indexs.push_back(vectors.size() - 2);
			indexs.push_back(vectors.size() - 4);
			indexs.push_back(vectors.size() - 3);
			indexs.push_back(vectors.size() - 1);
		}
	}
	indices = indexs;
	vertexes = vectors;
}
