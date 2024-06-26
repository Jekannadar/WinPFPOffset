#define CGAL_HAS_THREADS
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <queue>
#include <functional>
#include <set>
#include <memory>
#include <algorithm>
#include <bitset>
#include <thread>
#include "MeshKernel/Mesh.h"
#include "CGALPolygon.h"
#include "Eigen/Dense"
#include "Eigen/Core"
#include "Eigen/Sparse"
#include "OsqpEigen/OsqpEigen.h"
#include <fstream>
#include <limits>
#include <unordered_set>
#include <sstream>
#include "DSU.h"
#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Triangulation_2_projection_traits_3.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_2.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Polyhedron_items_with_id_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Polygon_mesh_processing/stitch_borders.h>
#include <CGAL/Polygon_mesh_processing/repair_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/merge_border_vertices.h>
#include <CGAL/Polygon_mesh_processing/manifoldness.h>
#include <CGAL/Polygon_mesh_processing/intersection.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include <CGAL/version.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Kd_tree.h>
#include <CGAL/Fuzzy_sphere.h>
#include <iostream>
#include <CGAL/Search_traits_3.h>
#include <atomic>
#include "obj_input.h"
#include "grid.h"
#include "global_face.h"
#include "geometric_data.h"
#include "cdt.h"
#include "unique_hash.h"
#include "CoverageField.h"
#include "osqp.h"
#include "dp.h"
#include "sort_by_polar_order.h"
#include "remeshing.h"
#include "single_coverage_ray_detect.h"
#include "flag_parser.h"
#include "merge_initial.h"
#undef small
//using namespace std;
//const int __gmp_bits_per_limb = 64;
int main(int argc, char* argv[]) {

    gflags::ParseCommandLineFlags(&argc, &argv, true);
    flag_parser();

    std::cout <<"CGAL_RELEASE_DATE:" << CGAL_RELEASE_DATE << std::endl;
    mesh = std::make_shared<MeshKernel::SurfaceMesh>(ReadObjFile(input_filename)); grid_len = 0.1;
    input_filename = input_filename.substr(0,input_filename.size()-4);
    input_filename+= std::string("_") + (running_mode==1?"offset_outward":"offset_inward");
    FILE *file11 = fopen( (input_filename + "_grid.obj").c_str(), "w");
    FILE *file6 = fopen( (input_filename + "_result.obj").c_str(), "w");
    FILE *file14 = fopen( (input_filename + "_coveragefield.obj").c_str(), "w");



    mesh->initBBox();
   // mesh->build_fast();
    auto start_clock = std::chrono::high_resolution_clock::now();
    std::cout <<"mesh->build_fast() succ" << std::endl;
    //double default_move_dist = 0.05;
    double x_len = (mesh->BBoxMax - mesh->BBoxMin).x();
    double y_len = (mesh->BBoxMax - mesh->BBoxMin).y();
    double z_len = (mesh->BBoxMax - mesh->BBoxMin).z();
    {
        double sum = 0;
        for(int i=0;i<mesh->FaceSize();i++){
            double minx = *std::set<double>{(mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(0)] - mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(1)]).norm(),
                                       (mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(1)] - mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(2)]).norm(),
                                       (mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(2)] - mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(0)]).norm()
            }.begin();
            double maxx = *std::set<double>{(mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(0)] - mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(1)]).norm(),
                                       (mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(1)] - mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(2)]).norm(),
                                       (mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(2)] - mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(0)]).norm()
            }.rbegin();

            sum += min(minx,maxx);

        }
        avg_edge_limit = sum/mesh->FaceSize();
        if(default_move <= 0) {
            default_move = sum/mesh->FaceSize()/2;
            std::cout <<"default_move_dist:" <<default_move << std::endl;
            //exit(0);
        }
        double min_len = min(min(x_len,y_len),z_len);
        double rate = x_len/min_len*y_len/min_len*z_len/min_len;

        std::cout <<"??"<<(4*thread_num) <<" "<<rate << std::endl; // minlen rate 格了 4* thread_num 格子
        double need_div = max(cbrt((4*thread_num)/rate),1.0);
        grid_len = min_len / need_div;//(mesh->BBoxMax - mesh->BBoxMin).norm()/ thread_num * 2;
        std::cout << "grid_len "<< grid_len<<std::endl;

    }
//    FILE * file50 = fopen((input_filename.substr(0,input_filename.size()-4) + "_offset.obj2").c_str(),"w");
//
//    fprintf(file50,"#download from quad mesh, add offset distance of every by PFPOffset.\n");
//    for (int i = 0; i < mesh->VertexSize(); i++) {
//        //std::cout << mesh->fast_iGameVertex[i].x() <<" "<< mesh->fast_iGameVertex[i].y() <<" "<<  mesh->fast_iGameVertex[i].z()<<std::endl;
//        fprintf(file50,"v %.7lf %.7lf %.7lf\n",mesh->fast_iGameVertex[i].x(),
//                mesh->fast_iGameVertex[i].y(),
//                mesh->fast_iGameVertex[i].z());
//    }
    for (int i = 0; i < mesh->FaceSize(); i++) {
        auto v0 = mesh->fast_iGameVertex[mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].vh(0)];
        auto v1 = mesh->fast_iGameVertex[mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].vh(1)];
        auto v2 = mesh->fast_iGameVertex[mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].vh(2)];
        auto tmp = (v0 + v1 + v2)/3;
        if(mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].move_dist <= 0){// deal Illegal input
            mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].move_dist = default_move;
        }
        if(i%1000 == 0){
            std::cout << "Imported " << i << "/" <<  mesh->FaceSize() << std::endl;
        }
        //Printing this much is really expensive
        //std::cout <<"facei "<<  mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].move_dist<<" "<<mesh->FastNeighborFhOfFace_[i].size()<< std::endl;
        //myeps = min(myeps,mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].move_dist);
//        fprintf(file50,"f %d %d %d %.7lf\n",mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].vh(0)+1,
//                mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].vh(1)+1,
//                mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].vh(2)+1,
//                ((tmp.x() - mesh->BBoxMin.x())/(mesh->BBoxMax.x() - mesh->BBoxMin.x())/2+0.5)*default_move
//                );

        // mesh->fast_iGameFace[MeshKernel::iGameFaceHandle(i)].move_dist =  default_move_dist;
        //FILE *file14 = fopen( (input_filename + "_coveragefield.obj").c_str(), "w");
    }

  //  exit(0);


    field_move_vertex.resize(mesh->VertexSize());
    field_move_vertices.resize(mesh->VertexSize());


    field_move_face.resize(mesh->FaceSize());
    field_move_K2_triangle.resize(mesh->FaceSize());

    std::cout <<"st do_quadratic_error_metric" << std::endl;


    merge_limit.resize(mesh->VertexSize());
    std::vector <std::shared_ptr<std::thread> > dp_thread_pool(thread_num);
    for(int i=0;i<thread_num;i++) {
        dp_thread_pool[i] = std::make_shared<std::thread>([&](int now_id) {
            for (int i = 0; i < mesh->VertexSize(); i++) {
            //for (int i = 524; i < 525; i++) {
                if (i % thread_num != now_id)continue;
                if (i % 20 == 0)
                    std::cout << "dp: " << i << "/"<<mesh->VertexSize()<< std::endl;
                std::vector<MeshKernel::iGameFaceHandle> neighbor_list;
                double min_move_dist = 1e30;
                for(auto j : mesh->FastNeighborFhOfVertex_[i]){
                    neighbor_list.push_back(j);
                }
                // std::cout <<i<<"//"<<mesh->VertexSize()<<" dp_result.size(): " <<"start" << std::endl;
                std::vector<MeshKernel::iGameVertex> dp_result = solve_by_dp(MeshKernel::iGameVertexHandle(i),neighbor_list);
                //std::cout <<i<<"//"<<mesh->VertexSize()<<" dp_result.size(): " <<dp_result.size() << std::endl;
                for(auto t: dp_result){
                    field_move_vertices[i].emplace_back(t.x(),t.y(),t.z());
                    min_move_dist = min(min_move_dist,mesh->fast_iGameVertex[i].dist(t));
                }

                merge_limit[i] = min_move_dist/10;
            }
        }, i);
    }
    for(int i=0;i<thread_num;i++)
        dp_thread_pool[i]->join();


    merge_initial();


    for(int i=0;i<mesh->FaceSize();i++){
        coverage_field_list.push_back(CoverageField(MeshKernel::iGameFaceHandle(i)));
    }
    int pre_cnt = 0;
    for(int i=0;i<mesh->FaceSize();i++){
//        if(coverage_field_list[i].bound_face_vertex_inexact.size() == 7 ) {
        for (int j = 0; j < coverage_field_list[i].bound_face_vertex_inexact.size(); j++) {
            fprintf(file14,"v %lf %lf %lf\n",CGAL::to_double(coverage_field_list[i].bound_face_vertex_inexact[j].x()),
                    CGAL::to_double(coverage_field_list[i].bound_face_vertex_inexact[j].y()),
                    CGAL::to_double(coverage_field_list[i].bound_face_vertex_inexact[j].z()));
        }
        for(int j=0;j<coverage_field_list[i].bound_face_id.size();j++){
            fprintf(file14,"f %d %d %d\n",coverage_field_list[i].bound_face_id[j][0]+1+pre_cnt,
                    coverage_field_list[i].bound_face_id[j][1]+1+pre_cnt,
                    coverage_field_list[i].bound_face_id[j][2]+1+pre_cnt);
        }
        pre_cnt += coverage_field_list[i].bound_face_vertex_inexact.size();
//            break;
//        }
    }
//    fclose(file14);
//    exit(0);

    std::vector <std::shared_ptr<std::thread> > one_ring_select_thread_pool(thread_num);
    for(int i=0;i<thread_num;i++) {
        one_ring_select_thread_pool[i] = std::make_shared<std::thread>([&](int now_id) {
            for(int i=0;i<mesh->FaceSize();i++) {
                if (i % thread_num != now_id)continue;
                if(i%500==0)
                    std::cout << "one_ring_select_thread_pool "<< i << std::endl;

                std::set<int>neighbor_field;

                for (auto neighbor_id: mesh->FastNeighborFhOfFace_[i]) {
                    if(neighbor_id == i)continue;
                    neighbor_field.insert(neighbor_id);
                }


                std::vector<K2::Triangle_3>neighbor_face;
                for(auto neighbor_id: neighbor_field){
                    for (int k = 0; k < coverage_field_list[neighbor_id].bound_face_id.size(); k++) {
                        K2::Triangle_3 tri_this(coverage_field_list[neighbor_id].bound_face_vertex_exact[coverage_field_list[neighbor_id].bound_face_id[k][0]],
                                                coverage_field_list[neighbor_id].bound_face_vertex_exact[coverage_field_list[neighbor_id].bound_face_id[k][1]],
                                                coverage_field_list[neighbor_id].bound_face_vertex_exact[coverage_field_list[neighbor_id].bound_face_id[k][2]]
                        );

                        neighbor_face.push_back(tri_this);
                    }
                }

                for(int j=0;j<coverage_field_list[i].bound_face_id.size();j++) {
                    if ((coverage_field_list[i].bound_face_id[j][0] >= 3 ||
                         coverage_field_list[i].bound_face_id[j][1] >= 3 ||
                         coverage_field_list[i].bound_face_id[j][2] >= 3)){
                        bool flag = false;

                        K2::Triangle_3 tri_this(coverage_field_list[i].bound_face_vertex_exact[coverage_field_list[i].bound_face_id[j][0]],
                                                coverage_field_list[i].bound_face_vertex_exact[coverage_field_list[i].bound_face_id[j][1]],
                                                coverage_field_list[i].bound_face_vertex_exact[coverage_field_list[i].bound_face_id[j][2]]);
                        K2::Segment_3 e0(tri_this.vertex(0), tri_this.vertex(1));
                        K2::Segment_3 e1(tri_this.vertex(1), tri_this.vertex(2));
                        K2::Segment_3 e2(tri_this.vertex(2), tri_this.vertex(0));

                        std::vector<K2::Segment_3> vs_tmp;
                        for (auto other: neighbor_face) { //Point_3, or Segment_3, or Triangle_3, or std::vector < Point_3 >
                            CGAL::cpp11::result_of<K2::Intersect_3(K2::Triangle_3, K2::Triangle_3)>::type
                                    res_tt = intersection(tri_this, other);
                            if (res_tt) {
                                if (const K2::Segment_3 *s = boost::get<K2::Segment_3>(&*res_tt)) {
                                    vs_tmp.push_back(*s);
                                } else if (const K2::Triangle_3 *t = boost::get<K2::Triangle_3>(&*res_tt)) {
                                    vs_tmp.emplace_back(t->vertex(0), t->vertex(1));
                                    vs_tmp.emplace_back(t->vertex(1), t->vertex(2));
                                    vs_tmp.emplace_back(t->vertex(2), t->vertex(0));
                                } else if (std::vector<K2::Point_3> *vs = boost::get<std::vector<K2::Point_3 >>(&*res_tt)) {
                                    sort_by_polar_order(*vs, tri_this.supporting_plane().orthogonal_vector());
                                    for (int k = 0; k < vs->size(); k++) {
                                        vs_tmp.emplace_back(vs->operator[](k), vs->operator[]((k + 1) % vs->size()));
                                        // cerr << "run iiiiiiiiiiiiiiit" << std::endl;
                                    }
                                }
                            }
                        }
                        std::vector<K2::Segment_3> vs;
                        for (auto se: vs_tmp) {
                            if (!segment_in_line(se, e0) && !segment_in_line(se, e1) && !segment_in_line(se, e2)) {
                                vs.push_back(se);
                            }
                        }
                        std::vector<std::vector<K2::Point_3> > res = CGAL_CDT_NEW({coverage_field_list[i].bound_face_vertex_exact[coverage_field_list[i].bound_face_id[j][0]],
                                                                         coverage_field_list[i].bound_face_vertex_exact[coverage_field_list[i].bound_face_id[j][1]],
                                                                         coverage_field_list[i].bound_face_vertex_exact[coverage_field_list[i].bound_face_id[j][2]]}, vs, tri_this);
                        //std::cout << res.size() <<std::endl;
                        for (auto each_tri: res) {
                            bool patch_flag = false;
                            K2::Point_3 center = CGAL::centroid(K2::Triangle_3(each_tri[0], each_tri[1], each_tri[2]));
                            for (auto j: neighbor_field) {
                                if (tri_this.supporting_plane().oriented_side(coverage_field_list[i].center) !=
                                    tri_this.supporting_plane().oriented_side(coverage_field_list[j].center) &&
                                    tri_this.supporting_plane().oriented_side(coverage_field_list[i].center)+
                                    tri_this.supporting_plane().oriented_side(coverage_field_list[j].center) ==0 &&
                                    coverage_field_list[j].in_or_on_field(center)) {
                                    patch_flag = true;
                                    break;
                                }
                            }
                            if(!patch_flag){
                                flag = true;
                                break;
                            }
                        }
                        coverage_field_list[i].bound_face_useful[j]=flag;
                        //std::cout <<i<<" "<<j <<" "<< (flag?"yes":"no") << std::endl;
                    }
                    else{
                        coverage_field_list[i].bound_face_useful[j]=false;
                    }

                }
            }
        },i);
    }

    for(int i=0;i<thread_num;i++)
        one_ring_select_thread_pool[i]->join();

    for(auto  each_container_face : container_grid_face){
        each_grid_face_list.push_back({(size_t)each_container_face[0],(size_t)each_container_face[1],(size_t)each_container_face[2]});
        each_grid_face_list.push_back({(size_t)each_container_face[2],(size_t)each_container_face[3],(size_t)each_container_face[0]});
    }

    std::cout <<"build end "<< std::endl;

    cgal_polygon = std::make_shared<CGALPolygon>(mesh);

    std::list < K2::Triangle_3> triangles;

    mesh->initBBox();

    double max_move = 0;
    double min_move = 1e10;
    for (auto i: mesh->fast_iGameFace) {
        max_move = max(max_move, abs(i.move_dist));
        min_move = min(min_move, abs(i.move_dist));
    }

    stx = mesh->BBoxMin.x() - max_move;
    sty = mesh->BBoxMin.y() - max_move;
    stz = mesh->BBoxMin.z() - max_move;

    printf("GL %lf\n", grid_len);

    std::unordered_map <grid, GridVertex, grid_hash, grid_equal> frame_grid_mp;

    std::vector <std::vector<int>> bfs_dir = {{0,  0,  1},
                                    {0,  1,  0},
                                    {1,  0,  0},
                                    {0,  0,  -1},
                                    {0,  -1, 0},
                                    {-1, 0,  0}};
    std::function < std::vector<grid>(grid) > get_neighbor = [&](grid g) {
        std::vector <grid> ret;
        for (auto i: bfs_dir) {
            int xx, yy, zz;
            xx = g.x + i[0];
            yy = g.y + i[1];
            zz = g.z + i[2];
            if (xx >= 0 && yy >= 0 && zz >= 0)
                ret.push_back({xx, yy, zz});
        }
        return ret;
    };
    int fsize = mesh->FaceSize();

    std::cout <<"bfs start \n" << std::endl;
    std::mutex bfs_mutex;
    std::vector <std::shared_ptr<std::thread> > bfs_thread_pool(thread_num);
    for(int i=0;i<thread_num;i++) {
        bfs_thread_pool[i] = std::make_shared<std::thread>([&](int now_id) {
            for (int face_id = 0; face_id < fsize; face_id++) {
                if(face_id % thread_num !=  now_id)continue;
                if (face_id % 1000 == 0)
                    printf("%d/%d\n", face_id, fsize);
                auto fh = std::make_pair(MeshKernel::iGameFaceHandle(face_id),
                                    mesh->fast_iGameFace[face_id]);
                MeshKernel::iGameVertex center = (mesh->fast_iGameVertex[fh.second.vh(0)] +
                                                  mesh->fast_iGameVertex[fh.second.vh(1)] +
                                                  mesh->fast_iGameVertex[fh.second.vh(2)]) / 3;

                double move_limit = 0;
                for(int j=0;j<3;j++){
                    for(int k=0;k<field_move_vertices[j].size();k++){
                        move_limit = max(move_limit,(Point_K_to_iGameVertex(field_move_vertices[fh.second.vh(j)][k]) - mesh->fast_iGameVertex[fh.second.vh(j)]).norm());
                    }
                }

                grid now = vertex_to_grid(center);
                std::vector <MeshKernel::iGameFaceHandle> face_and_neighbor;
                face_and_neighbor.push_back(fh.first);
                for (auto i: mesh->FastNeighborFhOfFace_[fh.first]) {
                    face_and_neighbor.push_back(i);
                }


                std::queue <grid> q;
                std::unordered_set <grid,grid_hash,grid_equal> is_visit;
                std::vector <grid> center_neighbor = get_neighbor(now);
                for (auto bfs_start_node: center_neighbor) {
                    is_visit.insert(bfs_start_node);
                    q.push(bfs_start_node);
                }
                while (!q.empty()) {
                    now = q.front();
                    q.pop();
                    //std::cout << now.x <<" "<< now.y <<""
                    std::unique_lock<std::mutex>lock1(bfs_mutex,std::defer_lock);
                    lock1.lock();
                    auto iter = frame_grid_mp.find(now);
                    if (iter == frame_grid_mp.end()) {
                        iter = frame_grid_mp.insert(std::make_pair(now, GridVertex())).first;
                    }
                    iter->second.field_list.push_back(fh.first);
                    lock1.unlock();
                    std::vector <grid> neighbor = get_neighbor(now);
                    for (auto j: neighbor) {
                        if (!is_visit.count(j)) {
                            double dist = cgal_vertex_triangle_dist(fh.second, getGridVertex(j, 0), mesh);

                            if (dist <  (grid_len*1.74/2+move_limit) ) { //TODO : zheli youhua cheng pianyi juli de shiji jisuan
                                q.push(j);
                                is_visit.insert(j);
                            }
                        }
                    }
                }
            }
        },i);
    }

    for(int i=0;i<thread_num;i++)
        bfs_thread_pool[i]->join();
    std::cout <<"bfs end \n" << std::endl;

    auto frame_grid_mp_bk =  frame_grid_mp;
    for(auto i : frame_grid_mp_bk) {
        for(auto j : container_grid_dir){
            int nx = i.first.x+j[0];
            int ny = i.first.y+j[1];
            int nz = i.first.z+j[2];
            //if(nx>=0 && ny>=0 && nz>=0){
            for(auto z : i.second.field_list)
                frame_grid_mp[{nx,ny,nz}].field_list.push_back(z);
            // }
        }
    }

    std::vector <std::shared_ptr<std::thread> > each_grid_thread_pool(thread_num);
    for(int i=0;i<thread_num;i++) { //todo 存在偶发性多线程异常
        each_grid_thread_pool[i] = std::make_shared<std::thread>([&](int now_id) {
            int each_grid_cnt =-1;
            for (auto each_grid = frame_grid_mp.begin(); each_grid != frame_grid_mp.end(); each_grid++) { // todo 逻辑修改
                each_grid_cnt++;
                if(each_grid_cnt % thread_num != now_id)continue;
                if(each_grid_cnt % (thread_num) == now_id)
                    printf("thread num :%d each_grid_cnt %d/%d\n",now_id,each_grid_cnt,(int)frame_grid_mp.size());

                MeshKernel::iGameVertex grid_vertex = getGridVertex(each_grid->first, 0);

                sort(each_grid->second.field_list.begin(), each_grid->second.field_list.end());
                each_grid->second.field_list.resize(
                        unique(each_grid->second.field_list.begin(), each_grid->second.field_list.end()) -
                        each_grid->second.field_list.begin());


                K2::Point_3 small = iGameVertex_to_Point_K2(getGridVertex(each_grid->first, 0));
                K2::Point_3 big = iGameVertex_to_Point_K2(getGridVertex(each_grid->first, 7));

                std::vector<K2::Point_3> ps;
                for(int i=0;i<8;i++){
                    ps.push_back(getGridK2Vertex(small,big,i));
                }
                std::list<K2::Triangle_3 >  frame_faces_list;

                for(auto i : each_grid_face_list){
                    frame_faces_list.emplace_back(ps[i[0]],ps[i[1]],ps[i[2]]);
                }
                Tree frame_aabb_tree(frame_faces_list.begin(),frame_faces_list.end());
                CGAL::Polyhedron_3<K2> frame_poly;
                PMP::repair_polygon_soup(ps,each_grid_face_list,CGAL::parameters::all_default());
                PMP::orient_polygon_soup(ps,each_grid_face_list);
                PMP::polygon_soup_to_polygon_mesh(ps, each_grid_face_list, frame_poly, CGAL::parameters::all_default());

                for(int j = 0; j <  each_grid->second.field_list.size() ; j++){
                    int field_id = each_grid->second.field_list[j];
                    for(int k=0;k<coverage_field_list[field_id].bound_face_id.size();k++){

                        int v0_id = coverage_field_list[field_id].bound_face_id[k][0];
                        int v1_id = coverage_field_list[field_id].bound_face_id[k][1];
                        int v2_id = coverage_field_list[field_id].bound_face_id[k][2];

                        K2::Triangle_3 this_tri(coverage_field_list[field_id].bound_face_vertex_exact[v0_id],
                                                coverage_field_list[field_id].bound_face_vertex_exact[v1_id],
                                                coverage_field_list[field_id].bound_face_vertex_exact[v2_id]
                        );
                        //std::cout <<"useful: "<< coverage_field_list[field_id].bound_face_useful[k] << std::endl;
                        if(!coverage_field_list[field_id].bound_face_useful[k])continue;
                        if(check_triangle_through_grid(small,big,frame_poly,this_tri)){

                            each_grid->second.field_face_though_list[field_id].push_back(k);
                            each_grid->second.face_hash_id_map[this_tri.id()] = {field_id,k};
                            each_grid->second.build_aabb_tree_triangle_list.push_back(this_tri);

                        }

                    }
                }
                Tree this_grid_aabb_tree( each_grid->second.build_aabb_tree_triangle_list.begin(),
                                          each_grid->second.build_aabb_tree_triangle_list.end());
               // std::cout <<each_grid_cnt<<":::::::" <<each_grid->second.build_aabb_tree_triangle_list.size()<<std::endl;
                //X: 这里写查询 因为aabb树指针的bug，所以反着做，用每个格子去查询每个面

                for(auto item : each_grid->second.field_face_though_list){
                    int field_id = item.first;
                    for(int bound_face_id : item.second){
                        int v0_id = coverage_field_list[field_id].bound_face_id[bound_face_id][0];
                        int v1_id = coverage_field_list[field_id].bound_face_id[bound_face_id][1];
                        int v2_id = coverage_field_list[field_id].bound_face_id[bound_face_id][2];
                        K2::Triangle_3 this_tri(coverage_field_list[field_id].bound_face_vertex_exact[v0_id],
                                                coverage_field_list[field_id].bound_face_vertex_exact[v1_id],
                                                coverage_field_list[field_id].bound_face_vertex_exact[v2_id]
                        );

                        std::list< Tree::Intersection_and_primitive_id<K2::Triangle_3>::Type> intersections;

                        this_grid_aabb_tree.all_intersections(this_tri,std::back_inserter(intersections));

                        std::unique_lock<std::mutex> lock(std::mutex);
                        for(auto item : intersections) { //todo 这里改成批量插入
                            if(const K2::Segment_3 * s = boost::get<K2::Segment_3>(&(item.first))){

                                std::map<size_t,std::pair<int,int> >::iterator iter = each_grid->second.face_hash_id_map.find(item.second->id());
                                if(iter->second.first != field_id ) {
                                    coverage_field_list[field_id].bound_face_cutting_segment[bound_face_id].push_back(
                                            *s);
                                }
                            }
                            else if(const K2::Point_3 * p = boost::get<K2::Point_3>(&(item.first))){
                                std::map<size_t,std::pair<int,int> >::iterator iter = each_grid->second.face_hash_id_map.find(item.second->id());
                                if(iter->second.first != field_id ) {
                                    coverage_field_list[field_id].bound_face_cutting_point[bound_face_id].push_back(*p);
                                }
                            }
                            else if(const std::vector<K2::Point_3> * v = boost::get<std::vector<K2::Point_3> >(&(item.first)) ){
                                std::map<size_t,std::pair<int,int> >::iterator iter = each_grid->second.face_hash_id_map.find(item.second->id());
                                if(iter->second.first != field_id ){
                                    for(int j=0;j<v->size();j++){
                                        coverage_field_list[field_id].bound_face_cutting_point[bound_face_id].push_back(v->at(j));
                                    }
                                }
                            }
                        }
                        coverage_field_list[field_id].bound_face_cross_field_list[bound_face_id].push_back(each_grid->first);
                    }
                }
            }
            std::cout << "each_grid_cnt end thread num:"<< now_id<<std::endl;
        }, i);
    }

    for(int i=0;i<thread_num;i++)
        each_grid_thread_pool[i]->join();

    for (auto each_grid= frame_grid_mp.begin(); each_grid != frame_grid_mp.end(); each_grid++) {
        //if(!(each_grid->first.x == 26 && each_grid->first.y == 28 && each_grid->first.z == 9  ))continue;
        //if(!(each_grid->first.x == 19 && each_grid->first.y == 19 && each_grid->first.z == 1 ))continue;
        //if(!(each_grid->first.x == 2 && each_grid->first.y == 3 && each_grid->first.z == 0 ))continue;
        auto small  = getGridVertex(each_grid->first,0);
        auto big  = getGridVertex(each_grid->first,7);
        static int f3_id = 1;
        for (int ii = 0; ii < 7; ii++) {
            for (int jj = 0; jj < DirectedGridEdge[ii].size(); jj++) {
                int from = ii;
                int to = DirectedGridEdge[ii][jj];
                MeshKernel::iGameVertex fv = getGridiGameVertex(small, big, from);
                MeshKernel::iGameVertex tv = getGridiGameVertex(small, big, to);
                fprintf(file11, "v %lf %lf %lf\n", fv.x(), fv.y(), fv.z());
                fprintf(file11, "v %lf %lf %lf\n", tv.x(), tv.y(), tv.z());
                fprintf(file11, "l %d %d\n", f3_id, f3_id + 1);
                f3_id += 2;
            }
        }
    }


    std::vector <std::shared_ptr<std::thread> > field_vertex_numbering_thread_pool(thread_num);
    std::atomic<int>global_vertex_id_sum;
    for(int i=0;i<thread_num;i++) {
        field_vertex_numbering_thread_pool[i] = std::make_shared<std::thread>([&](int now_id) {
            for (int field_id = 0; field_id < fsize; field_id++) {
                if (field_id % thread_num != now_id)continue;
                if(field_id %(10*thread_num) ==0)
                    std::cout << "start do cdt "<< field_id <<"/"<<fsize << std::endl;
                coverage_field_list[field_id].field_id = field_id;
                coverage_field_list[field_id].do_cdt();
                coverage_field_list[field_id].renumber();
                global_vertex_id_sum+= coverage_field_list[field_id].renumber_bound_face_vertex.size();
            }
        },i);
    }
    for(int i=0;i<thread_num;i++)
        field_vertex_numbering_thread_pool[i]->join();

    //exit(0);
    global_vertex_list.resize(global_vertex_id_sum);

    int global_cnt = 0;
    for (int field_id = 0; field_id < fsize; field_id++) {
        for (int i = 0; i < coverage_field_list[field_id].renumber_bound_face_vertex.size(); i++) {

            coverage_field_list[field_id].renumber_bound_face_vertex_global_id[i] = global_cnt + i;
            global_vertex_list[global_cnt + i] =  coverage_field_list[field_id].renumber_bound_face_vertex[i];
        }
        global_cnt += coverage_field_list[field_id].renumber_bound_face_vertex.size();
    }

    int global_face_cnt = 0;
    for (int field_id = 0; field_id < fsize; field_id++) {
        for (int i = 0; i < coverage_field_list[field_id].renumber_bound_face_id.size(); i++) {
            int tv0 =  coverage_field_list[field_id].renumber_bound_face_vertex_global_id[coverage_field_list[field_id].renumber_bound_face_id[i][0]];
            int tv1 =  coverage_field_list[field_id].renumber_bound_face_vertex_global_id[coverage_field_list[field_id].renumber_bound_face_id[i][1]];
            int tv2 =  coverage_field_list[field_id].renumber_bound_face_vertex_global_id[coverage_field_list[field_id].renumber_bound_face_id[i][2]];
            //if(set<int>{tv0,tv1,tv2}.size() < 3)continue;
            global_face_cnt++;
        }
    }
    //exit(0);
    global_face_list.resize(global_face_cnt);
    global_face_cnt = 0;
    for (int field_id = 0; field_id < fsize; field_id++) {
        for (int i = 0; i < coverage_field_list[field_id].renumber_bound_face_id.size(); i++) {
            int tv0 =  coverage_field_list[field_id].renumber_bound_face_vertex_global_id[coverage_field_list[field_id].renumber_bound_face_id[i][0]];
            int tv1 =  coverage_field_list[field_id].renumber_bound_face_vertex_global_id[coverage_field_list[field_id].renumber_bound_face_id[i][1]];
            int tv2 =  coverage_field_list[field_id].renumber_bound_face_vertex_global_id[coverage_field_list[field_id].renumber_bound_face_id[i][2]];

            int useful = coverage_field_list[field_id].renumber_bound_face_useful[i];
            if(std::set<int>{tv0,tv1,tv2}.size() < 3)continue;
            for(int j =0 ; j < coverage_field_list[field_id].renumber_bound_face_cross_field_list[i].size();j++){
                grid cross_grid = coverage_field_list[field_id].renumber_bound_face_cross_field_list[i][j];
                frame_grid_mp[cross_grid].global_face_list.push_back(global_face_cnt);
            }
            coverage_field_list[field_id].renumber_bound_face_global_id[i] = global_face_cnt;
            global_face_list[global_face_cnt].field_id = field_id;
            global_face_list[global_face_cnt].idx0 = tv0;
            global_face_list[global_face_cnt].idx1 = tv1;
            global_face_list[global_face_cnt].idx2 = tv2;
            global_face_list[global_face_cnt].useful = useful;
            global_face_list[global_face_cnt].center = CGAL::centroid(K2::Triangle_3(global_vertex_list[tv0],global_vertex_list[tv1],global_vertex_list[tv2]));
            global_face_cnt++;

        }
    }


    //exit(0);
    // 接下去就是求交

    std::cout << " 接下去时求交"<<std::endl;//deckel.obj
    std::queue<std::unordered_map <grid, GridVertex, grid_hash, grid_equal>::iterator > que;
    for (auto each_grid = frame_grid_mp.begin(); each_grid != frame_grid_mp.end(); each_grid++) {
        que.push(each_grid);
    }
    std::mutex que_mutex;
    std::vector <std::shared_ptr<std::thread> > face_generate_ray_detect_thread_pool(thread_num);
    for(int i=0;i<thread_num;i++) {
        face_generate_ray_detect_thread_pool[i] = std::make_shared<std::thread>([&](int now_id) {
            while(true) {
                std::unique_lock<std::mutex> lock(que_mutex);
                if (que.empty()) {
                    lock.unlock();
                    return;
                }
                auto each_grid = que.front();
                que.pop();
                printf("face_generate_ray_detect_thread_pool %d: %d/%d\n", now_id,
                       (int) frame_grid_mp.size() - que.size(), (int) frame_grid_mp.size());

                lock.unlock();
                std::unordered_map<unsigned long long, std::pair<int, int> > triangle_mapping;
                std::list<K2::Triangle_3> l;
                K2::Point_3 small = iGameVertex_to_Point_K2(getGridVertex(each_grid->first, 0));
                K2::Point_3 big = iGameVertex_to_Point_K2(getGridVertex(each_grid->first, 7));
                std::vector<K2::Point_3> ps;
                for (int i = 0; i < 8; i++) {
                    ps.push_back(getGridK2Vertex(small, big, i));
                }

                CGAL::Polyhedron_3<K2> frame_poly;
                PMP::repair_polygon_soup(ps,each_grid_face_list,CGAL::parameters::all_default());
                PMP::orient_polygon_soup(ps,each_grid_face_list);

                PMP::polygon_soup_to_polygon_mesh(ps, each_grid_face_list, frame_poly, CGAL::parameters::all_default());

                std::function<bool(K2::Point_3)> vertex_in_frame = [&](K2::Point_3 v) {
                    return small.x() <= v.x() && v.x() <= big.x() &&
                           small.y() <= v.y() && v.y() <= big.y() &&
                           small.z() <= v.z() && v.z() <= big.z();
                };

                for (int i = 0; i < each_grid->second.field_list.size(); i++) {
                    int field_id = each_grid->second.field_list[i];
                    bool useful = false;
                    if (coverage_field_list[field_id].in_field(midpoint(K2::Segment_3(ps[0], ps[7])))) {
                        useful = true;
                    }
                    if (!useful) {
                        if (vertex_in_frame(coverage_field_list[field_id].center)) {
                            useful = true;
                        }
                    }
                    if (!useful)
                        useful = CGAL::Polygon_mesh_processing::do_intersect(*coverage_field_list[field_id].poly,
                                                                             frame_poly);
                    if (useful) {
                        for (int j = 0; j < coverage_field_list[field_id].renumber_bound_face_global_id.size(); j++) {
                            int fid_global = coverage_field_list[field_id].renumber_bound_face_global_id[j];
                            K2::Point_3 v0 = global_vertex_list[global_face_list[fid_global].idx0];
                            K2::Point_3 v1 = global_vertex_list[global_face_list[fid_global].idx1];
                            K2::Point_3 v2 = global_vertex_list[global_face_list[fid_global].idx2];
                            K2::Triangle_3 tri(v0, v1, v2);
                            triangle_mapping[tri.id()] = {field_id, fid_global};
                            l.push_back(tri);
                        }
                    }
                }
                Tree aabb_tree(l.begin(), l.end());

                for (int i = 0; i < each_grid->second.global_face_list.size(); i++) {
                    int global_face_id = each_grid->second.global_face_list[i];
                    if (global_face_list[global_face_id].useful == false)continue;
                    K2::Point_3 v0 = global_vertex_list[global_face_list[global_face_id].idx0];
                    K2::Point_3 v1 = global_vertex_list[global_face_list[global_face_id].idx1];
                    K2::Point_3 v2 = global_vertex_list[global_face_list[global_face_id].idx2];

                    K2::Vector_3 ray_vec = K2::Triangle_3(v0, v1, v2).supporting_plane().orthogonal_vector();

                    K2::Ray_3 ray(global_face_list[global_face_id].center, -ray_vec);
                    std::list<Tree::Intersection_and_primitive_id<K2::Ray_3>::Type> intersections;
                    aabb_tree.all_intersections(ray, std::back_inserter(intersections));
                    for (auto item: intersections) {
                        std::pair<int, int> belong = triangle_mapping[item.second->id()];
                        int which_field = belong.first;
                        int which_id = belong.second;
                        if (which_field == global_face_list[global_face_id].field_id)continue;
                        if (const K2::Point_3 *p = boost::get<K2::Point_3>(&(item.first))) {
                            if (*p != global_face_list[global_face_id].center)
                                global_face_list[global_face_id].ray_detect_map[which_field].emplace_back(*p, which_id);
//                            else
//                                global_face_list[global_face_id].special_face_id.insert(which_field);
                        } else {
                            global_face_list[global_face_id].special_field_id.insert(which_field);
                        }
                    }
                }
            }

        },i);
    }
    for(int i=0;i<thread_num;i++)
        face_generate_ray_detect_thread_pool[i]->join();
    //exit(0);

    std::cout <<"start generate"<<std::endl;
    std::vector <std::shared_ptr<std::thread> > global_face_final_generate_thread_pool(thread_num);
    std::atomic<int> flag(0);
    for(int i=0;i<thread_num;i++) {
        global_face_final_generate_thread_pool[i] = std::make_shared<std::thread>([&](int now_id) {
            std::list<K2::Triangle_3>origin_face_list;
            for(int i=0;i<mesh->FaceSize();i++){
                K2::Point_3 v0(mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(0)].x(),
                               mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(0)].y(),
                               mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(0)].z());

                K2::Point_3 v1(mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(1)].x(),
                               mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(1)].y(),
                               mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(1)].z());

                K2::Point_3 v2(mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(2)].x(),
                               mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(2)].y(),
                               mesh->fast_iGameVertex[mesh->fast_iGameFace[i].vh(2)].z());
                origin_face_list.emplace_back(v0,v1,v2);
            }

            Tree origin_face_tree(origin_face_list.begin(),origin_face_list.end());

            for (int i = 0; i < global_face_list.size(); i++) {
                if(i %(thread_num*1000) ==0)
                    std::cout << "global_face_list:" << i <<"/"<<global_face_list.size()<< std::endl;
                if (i % thread_num != now_id)continue;
                if(global_face_list[i].useful == false )continue;
                for(std::unordered_map<int,std::vector<std::pair<K2::Point_3,int> > >::iterator it = global_face_list[i].ray_detect_map.begin();
                    it != global_face_list[i].ray_detect_map.end(); it++
                        ){////field 交点 对应的面
                    std::unordered_set<int>se;
                    std::vector<K2::Point_3> intersection_v;
                    for(int j=0;j<it->second.size();j++){
                        if(se.count(it->second[j].second))continue;
                        se.insert(it->second[j].second);
                        intersection_v.push_back(it->second[j].first);
                    }
                    sort(intersection_v.begin(),intersection_v.end(),[&](const K2::Point_3 &a,const K2::Point_3 &b){
                        if(a.x() != b.x()){
                            return a.x() < b.x();
                        }
                        else if(a.y() != b.y()){
                            return a.y() < b.y();
                        }
                        return a.z() < b.z();
                    });
                    int old_size = intersection_v.size();
                    intersection_v.resize(unique(intersection_v.begin(),intersection_v.end())-intersection_v.begin());
                    if(old_size != intersection_v.size()){
                        global_face_list[i].special_field_id.insert(it->first);
                        continue;
                    }
                    if(intersection_v.size()%2 == 1) {

                        global_face_list[i].useful = -200;
                        break;
                    }
                    // 1 是自己  // 若全无交点，可想而知一定在外，若有交点，那不一定在外，则正方向被覆盖，然后需要探测负方向，和自己
                    // 这里加入自己射线正负方向的寻找
                    // 2 出现多次 加入
                }


                if(  global_face_list[i].useful>0 ){
                    K2::Point_3 v0 = global_vertex_list[global_face_list[i].idx0];
                    K2::Point_3 v1 = global_vertex_list[global_face_list[i].idx1];
                    K2::Point_3 v2 = global_vertex_list[global_face_list[i].idx2];
                    if (running_mode == 2) {
                        if (!cgal_polygon->inMesh(centroid(K2::Triangle_3(v0, v1, v2)))) {
                            global_face_list[i].useful = -300;
                        }
                    }
                    else{
                        if (!cgal_polygon->outMesh(centroid(K2::Triangle_3(v0, v1, v2)))) {
                            global_face_list[i].useful = -300;
                        }
                    }

                    if(origin_face_tree.squared_distance(centroid(K2::Triangle_3(v0,v1,v2))) <= CGAL::Epeck::FT(0)){
                        global_face_list[i].useful = -300;
                    }
                }
            }
        },i);
    }

    for(int i=0;i<thread_num;i++)
        global_face_final_generate_thread_pool[i]->join();




    std::vector <std::shared_ptr<std::thread> > special_case_detect(thread_num);
    for(int i=0;i<thread_num;i++) {
        special_case_detect[i] = std::make_shared<std::thread>([&](int now_id) {
            for (int i = 0; i < global_face_list.size(); i++) {
                if (i % thread_num != now_id)continue;
                bool inner_flag = true;
                if(global_face_list[i].useful > 0 && !global_face_list[i].special_field_id.empty() )[[unlikely]]{
                    std::cout <<"meeting rare case, do special method"<<std::endl;
                    K2::Triangle_3 tri(global_vertex_list[global_face_list[i].idx0],
                                       global_vertex_list[global_face_list[i].idx1],
                                       global_vertex_list[global_face_list[i].idx2]);
                    for(auto field_id : global_face_list[i].special_field_id){
                        if(in_single_coverage_field_ray_detect(field_id,tri)){
                            inner_flag = false;
                            break;
                        }
                    }
                    if(!inner_flag){
                        global_face_list[i].useful = -200;
                    }
                }
            }
        },i);
    }
    for(int i=0;i<thread_num;i++)
        special_case_detect[i]->join();

    for(int i=0;i<global_vertex_list.size();i++){
        fprintf(file6,"v %lf %lf %lf\n",CGAL::to_double(global_vertex_list[i].x()),
                CGAL::to_double(global_vertex_list[i].y()),
                CGAL::to_double(global_vertex_list[i].z()));
    }

    double sum_avg_edge = 0;

    for (int i = 0; i < global_face_list.size(); i++) {
        if (i%1000==0)
            std::cout << "Wrote to result: " << i <<"/"<< global_face_list.size() << std::endl;
        if(global_face_list[i].useful>0) {
            if(running_mode == 1) {
                sum_avg_edge += sqrt(CGAL::to_double((CGAL::squared_distance(global_vertex_list[global_face_list[i].idx0] , global_vertex_list[global_face_list[i].idx1]))));
                sum_avg_edge += sqrt(CGAL::to_double((CGAL::squared_distance(global_vertex_list[global_face_list[i].idx2] , global_vertex_list[global_face_list[i].idx1]))));
                sum_avg_edge += sqrt(CGAL::to_double((CGAL::squared_distance(global_vertex_list[global_face_list[i].idx0] , global_vertex_list[global_face_list[i].idx2]))));

                fprintf(file6, "f %d %d %d\n", global_face_list[i].idx0 + 1, global_face_list[i].idx2 + 1,
                        global_face_list[i].idx1 + 1);
            }
            else {
                fprintf(file6, "f %d %d %d\n", global_face_list[i].idx0 + 1, global_face_list[i].idx1 + 1,
                        global_face_list[i].idx2 + 1);
            }
        }
    }
    fclose(file6);
    std::vector<K2::Point_3> points_ref;
    std::vector<std::vector<std::size_t> > faces_ref;
    std::ifstream in(input_filename + "_result.obj");
    CGAL::IO::read_OBJ(in,points_ref,faces_ref);
    CGAL::Polyhedron_3<K2> ms;
    PMP::repair_polygon_soup(points_ref,faces_ref);
    PMP::orient_polygon_soup(points_ref,faces_ref);
    PMP::polygon_soup_to_polygon_mesh(points_ref, faces_ref, ms);
    std::ofstream out(input_filename + "_result.stl",std::ios::binary);
    CGAL::IO::set_binary_mode(out);
    CGAL::IO::write_STL(out,ms);
    return 0;
}
// 1 2 3 4 5 6
// 5 1 2 3 7 8
//



