//
// Created by rainbowwing on 2023/8/25.
//

#ifndef THICKEN2_COVERAGEFIELD_H
#define THICKEN2_COVERAGEFIELD_H
struct CoverageField {
    std::vector<K2::Point_3 > bound_face_vertex_exact;
    std::vector<K::Point_3 > bound_face_vertex_inexact;
    std::vector<std::vector<int> > bound_face_id;
    std::vector<std::vector<grid> >  bound_face_cross_field_list;
    std::vector<std::vector<K2::Segment_3> > bound_face_cutting_segment;
    std::vector<std::vector<K2::Point_3> > bound_face_cutting_point;
    std::vector<bool> bound_face_useful;

    K2::Point_3 center;
    std::vector<std::vector<K2::Point_3> > cdt_result;
    std::vector<int>cdt_result_cross_field_list_id;
    std::vector<bool>cdt_result_cross_field_list_useful;

    std::vector<K2::Point_3 > renumber_bound_face_vertex;
    std::vector<std::vector<int> > renumber_bound_face_id;
    std::vector<std::vector<grid> > renumber_bound_face_cross_field_list;
    int field_id;
    void do_cdt(){
        //            std::vector<K2::Point_3> sorted_bound_vertex;
//            std::vector<K2::Segment_3> cs;
//            set<pair<int,int> >cs_set;
        for(int i=0;i<bound_face_id.size();i++){
            std::vector<K2::Point_3> sorted_bound_vertex{bound_face_vertex_exact[bound_face_id[i][0]],
                                                    bound_face_vertex_exact[bound_face_id[i][1]],
                                                    bound_face_vertex_exact[bound_face_id[i][2]]
            };
            if(!bound_face_useful[i]) {
                cdt_result.push_back(sorted_bound_vertex);
                cdt_result_cross_field_list_id.push_back(i);
                cdt_result_cross_field_list_useful.push_back(false);
                continue;
            }

            std::vector<K2::Segment_3> cs;
            for(auto j : bound_face_cutting_segment[i]){
                int cnt = 0;
                cnt += (j.vertex(0) == bound_face_vertex_exact[bound_face_id[i][0]] );
                cnt += (j.vertex(0) == bound_face_vertex_exact[bound_face_id[i][1]] );
                cnt += (j.vertex(0) == bound_face_vertex_exact[bound_face_id[i][2]] );
                cnt += (j.vertex(1) == bound_face_vertex_exact[bound_face_id[i][0]] );
                cnt += (j.vertex(1) == bound_face_vertex_exact[bound_face_id[i][1]] );
                cnt += (j.vertex(1) == bound_face_vertex_exact[bound_face_id[i][2]] );
                if(cnt <2 ){
                    cs.push_back(j);
                }
            }
            sort(cs.begin(),cs.end(),[&](const K2::Segment_3& a,const K2::Segment_3& b){
                if(a.vertex(0).x() != b.vertex(0).x()){
                    return a.vertex(0).x() < b.vertex(0).x();
                }
                else if(a.vertex(0).y() != b.vertex(0).y()){
                    return a.vertex(0).y() < b.vertex(0).y();
                }
                else if(a.vertex(0).z() != b.vertex(0).z()){
                    return a.vertex(0).z() < b.vertex(0).z();
                }
                else if(a.vertex(1).x() != b.vertex(1).x()){
                    return a.vertex(1).x() < b.vertex(1).x();
                }
                else if(a.vertex(1).y() != b.vertex(1).y()){
                    return a.vertex(1).y() < b.vertex(1).y();
                } else
                    return a.vertex(1).z() < b.vertex(1).z();
            });


            cs.resize(std::unique(cs.begin(),cs.end())-cs.begin());

            K2::Triangle_3 tri(bound_face_vertex_exact[bound_face_id[i][0]],
                               bound_face_vertex_exact[bound_face_id[i][1]],
                               bound_face_vertex_exact[bound_face_id[i][2]]);

            for(auto j : bound_face_cutting_point[i]){
                if(j != bound_face_vertex_exact[bound_face_id[i][0]] &&
                   j != bound_face_vertex_exact[bound_face_id[i][1]] &&
                   j != bound_face_vertex_exact[bound_face_id[i][2]]
                        )
                    sorted_bound_vertex.push_back(j);
            }
            sort(sorted_bound_vertex.begin(),sorted_bound_vertex.end(),[&](const K2::Point_3& a, const K2::Point_3& b){
                if(a.x() != b.x()){
                    return a.x() < b.x();
                }
                else if(a.y() != b.y()){
                    return a.y() < b.y();
                }
                return a.z() < b.z();
            });
            sorted_bound_vertex.resize(std::unique(sorted_bound_vertex.begin(),sorted_bound_vertex.end())-sorted_bound_vertex.begin());

            std::vector<std::vector<K2::Point_3> > res = CGAL_CDT_NEW(sorted_bound_vertex,cs,tri);

            for(int j=0;j<res.size();j++){
                cdt_result.push_back(res[j]);
                cdt_result_cross_field_list_id.push_back(i);
                cdt_result_cross_field_list_useful.push_back(true);
            }

        }

    }


    std::vector<int>renumber_bound_face_vertex_global_id;
    std::vector<int>renumber_bound_face_global_id;
    std::vector<bool>renumber_bound_face_useful;
    std::unordered_map<unsigned long long,int> encode_map;

    void renumber(){
        std::vector<K::Point_3> kd_tree_points;
        DSU dsu;
        std::map<K2::Point_3,int>mp;

        for(int i=0;i<cdt_result.size();i++){
            for(int j=0;j<3;j++){
                if(!mp.count(cdt_result[i][j])){
                    int size = mp.size();
                    mp[cdt_result[i][j]] = size;
                    renumber_bound_face_vertex.push_back(cdt_result[i][j]);
                }
            }
        }
        renumber_bound_face_vertex_global_id.resize(renumber_bound_face_vertex.size());



        //重构搜索要用原来的点
        for(int i=0;i<cdt_result.size();i++){

            int id0 = mp[cdt_result[i][0]];
            int id1 = mp[cdt_result[i][1]];
            int id2 = mp[cdt_result[i][2]];
            if(std::set<int>{id0,id1,id2}.size() != 3)continue;
            renumber_bound_face_id.push_back({id0,id1,id2});
            renumber_bound_face_cross_field_list.push_back(bound_face_cross_field_list[cdt_result_cross_field_list_id[i]]);
            renumber_bound_face_useful.push_back(cdt_result_cross_field_list_useful[i]);

        }
        std::list<K2::Triangle_3>tri_list;
        for(int i=0;i<renumber_bound_face_id.size();i++){
            tri_list.emplace_back(renumber_bound_face_vertex[renumber_bound_face_id[i][0]],
                                  renumber_bound_face_vertex[renumber_bound_face_id[i][1]],
                                  renumber_bound_face_vertex[renumber_bound_face_id[i][2]]);
        }
        Tree aabb_tree(tri_list.begin(),tri_list.end());
        auto iter = tri_list.begin();
        for(int i=0;i<renumber_bound_face_id.size();i++,iter++){
            K2::Point_3 tri_center = CGAL::centroid(*iter);
            K2::Ray_3 ray(tri_center,iter->supporting_plane().orthogonal_vector());
            std::list< Tree::Intersection_and_primitive_id<K2::Ray_3>::Type> intersections;
            aabb_tree.all_intersections(ray,std::back_inserter(intersections));
            std::vector<K2::Point_3> intersection_v;
            for(auto item : intersections) {
                if(const K2::Point_3* p = boost::get<K2::Point_3>(&(item.first))){
                    if(*p != tri_center){
                        intersection_v.push_back(*p);
                    }
                }
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
            intersection_v.resize(unique(intersection_v.begin(),intersection_v.end())-intersection_v.begin());
            if(intersection_v.size() % 2 == 0) { // 调整方向向外
                std::swap(renumber_bound_face_id[i][1],renumber_bound_face_id[i][2]);
            }
        }
        renumber_bound_face_global_id.resize(renumber_bound_face_id.size());
    }

    CoverageField(MeshKernel::iGameFaceHandle fh) {
        bound_face_vertex_inexact.emplace_back(mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(0)].x(),
                                               mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(0)].y(),
                                               mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(0)].z()
        );
        bound_face_vertex_inexact.emplace_back(mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(1)].x(),
                                               mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(1)].y(),
                                               mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(1)].z()
        );
        bound_face_vertex_inexact.emplace_back(mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(2)].x(),
                                               mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(2)].y(),
                                               mesh->fast_iGameVertex[mesh->fast_iGameFace[fh].vh(2)].z()
        );
        for(auto v: field_move_vertices[mesh->fast_iGameFace[fh].vh(0)])
            bound_face_vertex_inexact.emplace_back(v.x(),v.y(),v.z());
        for(auto v: field_move_vertices[mesh->fast_iGameFace[fh].vh(1)])
            bound_face_vertex_inexact.emplace_back(v.x(),v.y(),v.z());
        for(auto v: field_move_vertices[mesh->fast_iGameFace[fh].vh(2)])
            bound_face_vertex_inexact.emplace_back(v.x(),v.y(),v.z());


        std::map<size_t,int> mp;
        for(int i=0;i<bound_face_vertex_inexact.size();i++){

            mp[unique_hash_value(bound_face_vertex_inexact[i])] = i;
            bound_face_vertex_exact.emplace_back(bound_face_vertex_inexact[i].x(),
                                                 bound_face_vertex_inexact[i].y(),
                                                 bound_face_vertex_inexact[i].z());
        }
        Delaunay3D dt;
        dt.insert(bound_face_vertex_inexact.begin(), bound_face_vertex_inexact.end());
        std::vector<K::Triangle_3> surface_triangles;
        for (auto fit = dt.finite_cells_begin(); fit != dt.finite_cells_end(); ++fit) {
            for (int i = 0; i < 4; ++i) {
                if (dt.is_infinite(fit->neighbor(i))) {
                    surface_triangles.push_back(dt.triangle(fit, i));
                }
            }
        }


        K2::Vector_3 center_vec = {0,0,0};
        for (const auto& triangle : surface_triangles) {
            int v0_id = mp[unique_hash_value(triangle.vertex(0))];
            int v1_id = mp[unique_hash_value(triangle.vertex(1))];
            int v2_id = mp[unique_hash_value(triangle.vertex(2))];

            bound_face_id.push_back({v0_id, v1_id, v2_id});
            center_vec += (centroid(K2::Triangle_3(bound_face_vertex_exact[v0_id],
                                                   bound_face_vertex_exact[v1_id],
                                                   bound_face_vertex_exact[v2_id])) - K2::Point_3(0,0,0)) ;
        }
        center =  K2::Point_3(0,0,0) + (center_vec / surface_triangles.size());

        std::vector<std::vector<std::size_t> > faces_list;

        for (auto i: bound_face_id) {
            faces_list.push_back({std::size_t(i[0]), std::size_t(i[1]), std::size_t(i[2])});
            bound_face_useful.push_back(true);
        }
        bound_face_cross_field_list.resize(bound_face_id.size());
        bound_face_cutting_segment.resize(bound_face_id.size());
        bound_face_cutting_point.resize(bound_face_id.size());
        poly = new CGAL::Polyhedron_3<K2>();

        PMP::polygon_soup_to_polygon_mesh(bound_face_vertex_exact, faces_list, *poly, CGAL::parameters::all_default());
        inside_ptr = new CGAL::Side_of_triangle_mesh<CGAL::Polyhedron_3<K2>, K2>(*poly);

    }
public:
    bool in_field(K2::Point_3 v) {
        //CGAL::Side_of_triangle_mesh<CGAL::Polyhedron_3<K2>, K2> inside(*poly);
        if ((*inside_ptr)(v) == CGAL::ON_BOUNDED_SIDE)
            return true;
        return false;
    }

    bool in_or_on_field(K2::Point_3 v) {
        //CGAL::Side_of_triangle_mesh<CGAL::Polyhedron_3<K2>, K2> inside(*poly);
        auto side = (*inside_ptr)(v);
        if (side== CGAL::ON_BOUNDED_SIDE || side == CGAL::ON_BOUNDARY)
            return true;
        return false;
    }
    CGAL::Polyhedron_3<K2> * poly;
    CGAL::Side_of_triangle_mesh<CGAL::Polyhedron_3<K2>, K2> * inside_ptr;
};

std::vector<CoverageField> coverage_field_list;
#endif //THICKEN2_COVERAGEFIELD_H
