#include "map_process.h"
#include <ros/ros.h>

namespace zw {


void map_filter(char *out,uint32_t w, uint32_t h)
{
  int index;
  for(int x=1;x<w-1;x++)
  {
       index= GetGridIndexOfMap(w,x,0);
       if((out[index]==kFreeGrid)&&(out[index-1]!=kFreeGrid)&&
          (out[index+1]!=kFreeGrid)&&(out[index+w]!=kFreeGrid))
           out[index] =kUnknownGrid;
       index= GetGridIndexOfMap(w,x,h-1);
       if((out[index]==kFreeGrid)&&(out[index-1]!=kFreeGrid)&&
          (out[index+1]!=kFreeGrid)&&(out[index-w]!=kFreeGrid))
           out[index] =kUnknownGrid;
  }
  for(int y=1;y<h-1;y++)
  {
       index= GetGridIndexOfMap(w,0,y);
       if((out[index]==kFreeGrid)&&(out[index-w]!=kFreeGrid)&&
          (out[index+w]!=kFreeGrid)&&(out[index+1]!=kFreeGrid))
           out[index] =kUnknownGrid;
       index= GetGridIndexOfMap(w,w-1,y);
       if((out[index]==kFreeGrid)&&(out[index-w]!=kFreeGrid)&&
          (out[index+w]!=kFreeGrid)&&(out[index-1]!=kFreeGrid))
           out[index] =kUnknownGrid;
  }
  for(int y=1;y<h-1;y++)
  {
      for(int x=1;x<w-1;x++)
      {
          index= GetGridIndexOfMap(w,x,y);
          if((out[index]==kFreeGrid)&&(out[index-1]!=kFreeGrid)&&
             (out[index+1]!=kFreeGrid)&&(out[index+w]!=kFreeGrid)&&
             (out[index-w]!=kFreeGrid))
              out[index] =kUnknownGrid;
      }
  }
}

bool CellInfo_cmp(const CellInfo& c1,const CellInfo & c2)
 {
     if(c1.status==c2.status)
         return c1.grade > c2.grade;
     return c1.status > c2.status;
 }

MapProcess::MapProcess()
{
    free_grid_Cell.resize(0);
    free_space_count=0;
    map_resolution =0.05;
}

MapProcess::~MapProcess()
{

}

void MapProcess::GetBinaryAndSample(const nav_msgs::OccupancyGridConstPtr& grid ,int th_occ ,int th_free ,int num )
{
   int w=grid->info.width/num;
   int h=grid->info.height/num;
   map_resolution = grid->info.resolution *num;
   char *mapdata = new char[w*h];
   int index=0;
   for(int y=0;y<h;y++)
   {
       for(int x=0; x<w;x++)
       {
          int sum=0;
          for(int j=0;j<num;j++)
          {
              for(int i=0;i<num;i++)
              {
                  index= GetGridIndexOfMap(grid->info.width,(x*num+i),(y*num+j));
                  sum+= grid->data[index];
              }
          }
          if(sum>=th_occ)
              mapdata[y*w+x] = kOccGrid;
          else if(sum <th_free)
              mapdata[y*w+x] = kUnknownGrid;
          else
              mapdata[y*w+x] =kFreeGrid;
       }
   }

   map_filter(mapdata,w,h);
   free_space_count = GetFreeSpcaceIndices(mapdata,w,h);
   CalNeighbour(mapdata,w,h ,map_resolution);

   filter_map.info.resolution = map_resolution;
   filter_map.info.width = w;
   filter_map.info.height = h;
   filter_map.info.map_load_time=ros::Time::now();
   filter_map.header.frame_id="map";
   filter_map.header.stamp=ros::Time::now();
   filter_map.data.resize(w*h,-1);

   filter_map.info.origin.position.x =grid->info.origin.position.x;
   filter_map.info.origin.position.y =grid->info.origin.position.y;
   filter_map.info.origin.position.z =0;

   for (size_t k = 0; k < w*h; ++k)
       filter_map.data[k]=mapdata[k];

   delete mapdata;

   ROS_INFO("map info %d X %d map @ %3.2lf m/cell",filter_map.info.width,
            filter_map.info.height,filter_map.info.resolution);
   ROS_INFO("total cell=%d \nsample cell=%d  free cell=%d",grid->info.width*grid->info.height,
            w*h,free_space_count);
}

int MapProcess::GetFreeSpcaceIndices(const char *grid,int w,int h)
{
    CellInfo cell={0,0,0,0,0,{0,0,0,0,0,0,0,0}};

    for(int j = 0; j <h; j++)
        for(int i = 0; i < w; i++)
        {
            if(grid[GetGridIndexOfMap(w,i,j)]==kFreeGrid)
            {
                cell.x=i;
                cell.y=j;
                free_grid_Cell.push_back(cell);
            }
        }
    return (int)free_grid_Cell.size();
}

void MapProcess::CalNeighbour(const char *grid,int w,int h ,float resolution)
{
    int max_count = floor(kMaxLaserRange/resolution);
  //  int min_count = floor(kMinLaserRange/resolution);
    for(int i=0;i<free_space_count;i++)
    {
        int k=1;
        int x= free_grid_Cell[i].x;
        int y= free_grid_Cell[i].y;
        float sum=0;
        int count=0;
        while(k<=max_count)
        {
            if((y+k)<h)
            {
                 if(grid[GetGridIndexOfMap(w,x,y+k)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[0] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }
        k=1;
        while(k<=max_count)
        {
            if((x+k)<w)
            {
                 if(grid[GetGridIndexOfMap(w,x+k,y)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[2] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }
        k=1;
        while(k<=max_count)
        {
            if((y-k)>=0)
            {
                 if(grid[GetGridIndexOfMap(w,x,y-k)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[4] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }
        k=1;
        while(k<=max_count)
        {
            if((x-k)>=0)
            {
                 if(grid[GetGridIndexOfMap(w,x-k,y)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[6] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }

        resolution *=1.414213;
        max_count = floor(kMaxLaserRange/resolution);
        k=1;
        while(k<=max_count)
        {
            if((x+k)<w &&(y+k)<h)
            {
                 if(grid[GetGridIndexOfMap(w,x+k,y+k)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[1] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }
        k=1;
        while(k<=max_count)
        {
            if((x+k)<w &&(y-k)>=0)
            {
                 if(grid[GetGridIndexOfMap(w,x+k,y-k)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[3] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }
        k=1;
        while(k<=max_count)
        {
            if((x-k)>=0 &&(y-k)>=0)
            {
                 if(grid[GetGridIndexOfMap(w,x-k,y-k)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[5] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }
        k=1;
        while(k<=max_count)
        {
            if((x-k)>=0 &&(y+k)<h)
            {
                 if(grid[GetGridIndexOfMap(w,x-k,y+k)]==kOccGrid)
                 {
                     free_grid_Cell[i].neighbour[7] = k*resolution ;
                     sum += k*resolution ;
                     count++;
                     break;
                 }
            }
            k++;
        }

        free_grid_Cell[i].dis_avg =sum/count;
    }

//    for(int i=0;i<5;i++)
//    ROS_INFO("[x,y]=[%d,%d]  [%6.2f,%6.2f,%6.2f,%6.2f,%6.2f,%6.2f,%6.2f,%6.2f]",
//        free_grid_Cell[i].x,free_grid_Cell[i].y,
//        free_grid_Cell[i].neighbour[0],free_grid_Cell[i].neighbour[1],
//        free_grid_Cell[i].neighbour[2],free_grid_Cell[i].neighbour[3],
//        free_grid_Cell[i].neighbour[4],free_grid_Cell[i].neighbour[5],
//        free_grid_Cell[i].neighbour[6],free_grid_Cell[i].neighbour[7]);
}

void MapProcess::CalScan(const sensor_msgs::LaserScanConstPtr& scan ,float err)
{
    size_t size = scan->ranges.size();
    float angle = scan->angle_min;

    float sum=0;
    int count=0;

    for (size_t i = 0; i < size; ++i)
    {
        float dist = scan->ranges[i];
        if ( (dist >kMinLaserRange) && (dist < kMaxLaserRange))
        {
            sum +=dist ;
            count ++;
        }
        angle += scan->angle_increment;
    }
    singleScan.dis_avg =sum/count ;

    for(int i=0;i<free_space_count;i++)
    {
      if(abs(free_grid_Cell[i].dis_avg- singleScan.dis_avg)<err)
        free_grid_Cell[i].status =1;
    }

    std::sort(free_grid_Cell.begin(),free_grid_Cell.end(),CellInfo_cmp);
}

geometry_msgs::Point32 MapProcess::GetPoint(const CellInfo & cell ,const nav_msgs::OccupancyGrid& map)
{
    geometry_msgs:: Point32 p;

    p.x= map.info.origin.position.x +(cell.x+0.5)*map.info.resolution ;
    p.y= map.info.origin.position.y +(cell.y+0.5)*map.info.resolution ;
    p.z=0;
    return p;
}

}

