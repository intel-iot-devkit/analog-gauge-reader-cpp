#include <opencv2/core/base.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#define PI 3.14159265

using namespace std;
using namespace cv;


//Function to convert int to string. 
string to_string(int i)
{
        stringstream ss;
        ss << i;
        return ss.str();
}


//dist_2_pts function calculates the distance between the two points. 
float dist_2_pts(float x1,float y1,float x2,float y2)
{
        float distance = sqrt(pow((x2-x1),2) + pow((y2-y1),2));
        return distance;
}


//This function calibrates the range available to dial and as well as the units. It works by finding the center point and radius of the gauge. It returns the x coordinate, y coordinate and radius of the circle. 
vector<Vec3f> calibrate_gauge(int gauge_number, string file_type)
{
        string str = to_string(gauge_number);
        string file_name = str.append(file_type);

        //Read the image
        Mat image  = imread(file_name);
        Mat image_gray;

        if (!image.data)
        {
                cout << "ERROR:\topening image";
                exit(-1);
        }

        int p = int(image.rows*0.35) , q = int(image.rows*0.48);

        //Convert to gray
        cvtColor(image, image_gray, CV_BGR2GRAY );
        vector <Vec3f> circles;
	//Detect circles
        HoughCircles(image_gray, circles, HOUGH_GRADIENT, 1,20,90,70,p,q);

        for(size_t i = 0; i< circles.size() ; i++)
        {
                Point2f center(circles[i][0], circles[i][1]);
                float r = circles[i][2];

                //Draw the circle
                circle(image,center, r, Scalar( 0, 0, 255 ), 3, 8,0 );

                //Draw the center of the circle
                circle(image, center,3,Scalar(0,255,0),-1,8,0);

        }

        //Goes through the motion of a circle and sets x_coord and y_coord values based on the set separation spacing. Also adds text to each line.These lines and labels serve as the reference point for the user to enter. 
        int separation = 10.0;
        int interval = 360/separation;
        int pt_1[interval][2];

        for(size_t i = 0; i< circles.size() ; i++)
        {
                float x_coord(circles[i][0]), y_coord(circles[i][1]), r = circles[i][2];

                for (int i=0; i<interval; i++)
                {
                        for (int j=0; j<2; j++)
                        {
                                if(j%2==0)
                                        //Point for lines
                                        pt_1[i][j]= x_coord + 0.9*r*cos(separation*i* CV_PI/180) ;
                                else
                                        pt_1[i][j]= y_coord + 0.9*r*sin(separation*i* CV_PI/180);
                        }
                }

                int pt_2[interval][2] , pt_text[interval][2] , text_offset_x = 10 , text_offset_y =  5;
                for (int i=0; i<interval; i++)
                {
                        for (int j=0; j<2; j++)
                        {
                                if(j%2==0)
                                {

                                        pt_2[i][j]= x_coord + r*cos(separation*i* CV_PI/180);
                                        //Point for text labels 
                                        pt_text[i][j] = x_coord - text_offset_x + 1.2*r*cos((separation) * (i+9) * CV_PI/180);
                                }
                                else
                                {
                                        pt_2[i][j]= y_coord + r*sin(separation*i * CV_PI/180);
                                        pt_text[i][j] = y_coord + text_offset_y + 1.2*r*sin((separation)*(i+9) * CV_PI/ 180);
                                }
                        }
                }
 		//Add lines and labels to the image
                for(int i=0;i<interval;i++)
                {
                        line(image,Point2f(pt_1[i][0],pt_1[i][1]),Point2f(pt_2[i][0],pt_2[i][1]), Scalar(0,255,0),2);
                        string temp=to_string(i*separation);
                        putText(image,temp,Point2f(pt_text[i][0],pt_text[i][1]), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0,0,0),1,LINE_AA);
                }

                //Write the image
                string str = to_string(gauge_number);
                string cal_image = str.append(".calibration");
                string file_name = cal_image.append(file_type);
                imwrite(file_name,image);

                //Display an image
                imshow("Image",image);
                waitKey(0);


        }
        return circles;
}



//This function find the straight lines using HoughLinesP and uses the calibrated values which is provided by the user to convert the angle of the dial into a meaningful value. The value of the analog of the image is returned.  
float get_current_value(Mat image,float min_angle, float max_angle, float min_value, float max_value, float x, float y, float r, int gauge_number, string file_type)

{

        Mat image_gray, dst;
        cvtColor(image,image_gray,CV_BGR2GRAY );

        //Set threshold and max value
        int thresh = 175 , maxValue = 255;

        //Apply threshold which helps in finding lines
        threshold(image_gray,dst,thresh,maxValue, THRESH_BINARY_INV);
        
        int min_line_length = 10, max_line_gap = 0;
        vector<Vec4i> lines;

        //Detect straight lines
        HoughLinesP(dst,lines, 3,CV_PI/180, 100,min_line_length,max_line_gap);

        //diff1LowerBound and diff1UpperBound determine how close the line should be from the center
        float diff1_lower_bound = 0.10 , diff1_upper_bound = 0.30;

        //dif2LowerBound and diff2UpperBound determine how close the line should be from the center
        float diff2_lower_bound = 0.5 , diff2_upper_bound = 1.0;

        int column = 4;
        vector< vector<float> > final_line_list(column);
        vector<float> list;
  	float a[20];
        float diff1UB = float(diff1_upper_bound*r);
        float diff1LB = float(diff1_lower_bound*r);
        float diff2UB = float(diff2_upper_bound*r);
        float diff2LB = float(diff2_lower_bound*r);

        for(int i = 0; i<lines.size(); i++)
        {
                float x1, x2, y1, y2;
                x1 = lines[i][0];
                y1 = lines[i][1];
                x2 = lines[i][2];
                y2 = lines[i][3];

                //x, y is the center of circle
                float diff1 = dist_2_pts(x,y,x1,y1);
                float diff2 = dist_2_pts(x,y,x2,y2);
                if(diff1 > diff2)
                {
                        float temp = diff1;
                        diff1 = diff2;
                        diff2 = temp;
                }

                if(((diff1 < diff1UB) && (diff1 > diff1LB)) && ((diff2 < diff2UB) && (diff2 > diff2LB)))
                {

                        float line_length = dist_2_pts(x1, y1, x2, y2);
                        list.push_back(x1);
                        list.push_back(y1);
                        list.push_back(x2);
                        list.push_back(y2);

                        final_line_list.push_back(list);



                        for(int i = 0; i < final_line_list.size(); i++)

                        {
                                for(int j = 0; j < final_line_list[i].size(); j++)
                               {

                                        a[j] = final_line_list[i][j];

                               }
                        }
                }
        }

        float x1 = a[0] , y1 = a[1] , x2 = a[2] , y2 = a[3];

        line(image, Point2f(x1,y1),Point2f(x2,y2), Scalar(0,255,0),2);

        string str = to_string(gauge_number);
        string lines_image = str.append(".lines");
        string file_name = lines_image.append(file_type);
        imwrite(file_name,image);

        float dist_pt_0 = dist_2_pts(x,y,x1,y1);
        float dist_pt_1 = dist_2_pts(x,y,x2,y2);
	float x_angle, y_angle, final_angle;
        double div;

        if(dist_pt_0 > dist_pt_1)
        {
                x_angle = x1 - x;
                y_angle = y - y1;

        }

        else
        {
                x_angle = x2 - x;
                y_angle = y - y2;

        }


        div = y_angle/x_angle;

        //Take the arc tan of y/x to find the angle
        float res = atan(div);
        float res1 = res * 180/PI;

        //In quadrant I
        if(x_angle > 0 && y_angle > 0)
        {
                final_angle = 270-res1;
        }

        //In quadrant II
        if( x_angle < 0 && y_angle > 0)
        {
                final_angle = 90-res1;
        }

        //In quadrant III
        if( x_angle < 0 && y_angle < 0)
        {
                final_angle = 90-res1;
        }

        //In quadrant IV
        if( x_angle > 0 && y_angle < 0)
        {
                final_angle = 270-res;
        }

        float old_min,old_max,new_min,new_max,old_value,old_range,new_range,new_value;

        old_min = float(min_angle);
        old_max = float(max_angle);
        new_min = float(min_value);
        new_max = float(max_value);
        old_value = final_angle;
        old_range = (old_max-old_min);
        new_range = (new_max - new_min);
 	new_value = (((old_value - old_min) * new_range) / old_range) + new_min;

        return new_value;

}


//It prompts the user to enter lowest possible angle of dial, highest possible angle, minimum value of the gauge, the maximum value and the units(as a string) according to the calibrated image obtained from the function-calibrate_gauge().
int main(int argc , char **argv)
{
        Mat image_gray,image,dst;
        int gauge_number = 1;
        float min_angle, max_angle,min_value, max_value, val,x,y,r;
        string units , file_type= ".jpg";
        vector <Vec3f> circle;

        circle = calibrate_gauge(gauge_number,file_type);
        x = circle[0][0];
        y = circle[0][1];
        r = circle[0][2];

        cout<<"Min angle (lowest possible angle of dial) - in degrees " <<endl;   //48
        cin >> min_angle;
        cout<<"Max angle (highest possible angle) - in degrees: " <<endl;  //319
        cin >> max_angle;
        cout<<"Min value" << endl;   //0
        cin >> min_value;
        cout<<"Max value" << endl;   //200
        cin >> max_value;
        cout<< "Enter units" <<endl;  //psi
        cin >> units;

        string str = to_string(gauge_number);
        string file_name = str.append(file_type);
        image= imread(file_name);

        val = get_current_value(image, min_angle, max_angle, min_value, max_value, x, y, r, gauge_number, file_type);
        cout<<"current reading is " <<val <<" " << units <<endl;

        return 0;
}


