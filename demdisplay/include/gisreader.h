#define GLM_ENABLE_EXPERIMENTAL
#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ogrsf_frmts.h>

using namespace std;
using namespace glm;
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

bool getRawValuesFromFile(string fname,vector<vector<float>>& vecs,float& min, float& max,float& xres, float& yres, string& projection,double& XORIGIN,double& YORIGIN,int& W, int& H);
void createMesh(vector<vector<float>>& input,float xres,float yres,float max, vector<int>& indicies, vector<Vertex>& vertexes);
void bilinearintertop(vector<vector<float>>& input, int width, int height);
