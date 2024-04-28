//
// Created by rainbowwing on 2023/8/25.
//

#ifndef THICKEN2_GLOBAL_FACE_H
#define THICKEN2_GLOBAL_FACE_H
struct GlobalFace{
    int idx0,idx1,idx2;
    int field_id;
    int useful;
    K2::Point_3 center;
    std::set<int>special_field_id;
    // 特例： 交点是自己
    // 交点是二合一点
    //

    std::unordered_map<int,std::vector<std::pair<K2::Point_3,int> > > ray_detect_map;//field 交点 int
};
std::vector<MeshKernel::iGameVertex> field_move_vertex;
std::vector<std::vector<K::Point_3> > field_move_vertices;
std::vector<std::vector<MeshKernel::iGameVertex> > field_move_face;
std::vector<K2::Triangle_3> field_move_K2_triangle;
std::vector<K2::Point_3> global_vertex_list;
std::vector<int> global_vertex_list_cnt;
std::vector<K2::Vector_3> global_vertex_list_avg;
std::vector<GlobalFace> global_face_list;
std::vector<double>merge_limit;
#endif //THICKEN2_GLOBAL_FACE_H
