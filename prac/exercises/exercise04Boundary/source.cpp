#include <iostream>

#include <Core/EntryPoint.h>
#include <ATCG.h>

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>


class Exercise04Layer : public atcg::Layer
{
public:
    Exercise04Layer(const std::string& name) : atcg::Layer(name) {}

    Eigen::SparseMatrix<int> compute_boundary_operator(const std::shared_ptr<atcg::Mesh>& mesh)
    {
        /// Exercise: -Implement the \partial_2 boundary operator of the given mesh
        ///           -Define a fixed order for the edges
        ///           -You can use Eigen::SparseMatrix<int>::setFromTriplets to build the sparse matrix
        ///           -You can use eh.idx() to get a unique index for each edge (likewise for faces and vertices)
        ///           -DO NOT use OpenMesh functions to retreive boundary information (like mesh->is_boundary(eh) or
        ///           mesh->is_boundary(vh)). Only retreive this information from the orientation of edges. Of course,
        ///           in practice you wouldn't do it this way but this is about the learning experience.

        // 
        
        size_t ne = mesh->n_edges(); // number edges
        size_t nf = mesh->n_faces(); // number faces

        Eigen::SparseMatrix<int> d2(ne, nf);

        std::vector<Eigen::Triplet<int>> triplets;

        for (auto f_it = mesh->faces_begin(); f_it != mesh->faces_end(); ++f_it)
        {
            for (auto e : mesh->fe_range(f_it))  // Iterate over edges of face
            {
                auto he0 = mesh->halfedge_handle(e, 0);  // First halfedge
                auto he1 = mesh->halfedge_handle(e, 1);  // Second halfedge

                // check halfedge belonging to current face
                if (mesh->face_handle(he0) == *f_it){
                    triplets.push_back({e.idx(), f_it->idx(), 1});}  // good Orientation
                if (mesh->face_handle(he1) == *f_it)
                    triplets.push_back({e.idx(), f_it->idx(), -1});  // nah orientation
            }
        }

        d2.setFromTriplets(triplets.begin(), triplets.end());

        // std::cout << "d2 shape: (" << d2.rows() << "," << d2.cols() << ")" << std::endl; //!!DEBUG

        // COOOOL solution
        return d2;
    }

    Eigen::VectorXi detect_boundary(const Eigen::SparseMatrix<int>& M, const Eigen::VectorXi& v)
    {
        /// Exercise: -Implement the slicing of a mesh with a vector v
        ///           -The output should be a vector with n_vertices() entries where non-boundary vertices have value 0
        ///           -OpenMesh of course has methods to get boundary vertices directly. DO NOT use those.

        Eigen::VectorXi edge_sums = M * v;
        return (edge_sums.array() == 1).cast<int>();

        // 

        // Dummy solution
        return Eigen::VectorXi();
    }

    Eigen::VectorXi compute_boundary_edges(const Eigen::SparseMatrix<int>& M)
    {
        /// Exercise: -Get a vector with n_edges() entries where non-boundary edges have value 0
        ///           -OpenMesh of course has methods to get boundary edges directly. DO NOT use those.

        // 

        Eigen::VectorXi edge_count = Eigen::VectorXi::Zero(M.rows());

        std::cout << "M.rows(): " << M.rows() << std::endl;

        for (int i = 0; i < M.rows(); ++i){

            for (int k = 0; k < M.cols(); ++k) // Obviously we have more than two faces other than the example... -_-
                edge_count[i] += M.coeff(i, k);
        }

        // Cooked solution
        return edge_count;
    }

    Eigen::VectorXi compute_plane_cut(const std::shared_ptr<atcg::Mesh>& mesh, const glm::vec3& plane_normal)
    {
        /// Exercise: -Get a vector with n_faces() entries where faces that are cut by the plane have value 1 and all
        ///            other faces have value 0
        ///           -The plane is defined by the normal vector 'plane_normal' and the origin (0,0,0)
        ///           -Think about a sensible decision criterion to determine if a face is cut by the plane.
        ///           -OpenMesh of course has methods to get boundary edges directly. DO NOT use those.

        // 

         Eigen::VectorXi cut_faces = Eigen::VectorXi::Zero(mesh->n_faces());

        for (auto f_it = mesh->faces_begin(); f_it != mesh->faces_end(); ++f_it) {
            // check if any vertex is on opposite sides of plane
            bool has_positive = false;
            bool has_negative = false;

            for (auto fvh : mesh->fv_range(*f_it)) { // Iterate over face vertices
                auto pos = mesh->point(fvh);
                float side = glm::dot(glm::vec3(pos[0], pos[1], pos[2]), plane_normal);
                if (side >= 0) {has_positive = true;}
                if (side < 0) {has_negative = true;}
            }

            if (has_positive && has_negative) {
                cut_faces[(*f_it).idx()] = 1;  // face is cut by plane
            }
        }

        return cut_faces;


        // Unused "solution"
        return Eigen::VectorXi();
    }

    // 🖌️🖌️🖌️🖌️
    void color_mesh(const std::shared_ptr<atcg::Mesh>& mesh, const Eigen::VectorXi& sum)
    {
        /// Exercise: - Color all vertices that make up the boundary edges red
        ///           - This framework does not support to color in edges directly, so you have to color in the
        ///           respective vertices 
        ///           - sum contains the output of 'compute_boundary_edges'.
        ///           - Again, DO NOT use OpenMesh functions to detect boundaries.

        std::cout << "sum shape: (" << sum.size() << ")" << std::endl;

        for (auto v_it = mesh->vertices_begin(); v_it != mesh->vertices_end(); ++v_it){
            // Coloring verts of edge         
            for (auto ve_it = mesh->ve_iter(*v_it); ve_it.is_valid(); ++ve_it) {
                atcg::Mesh::EdgeHandle eh = *ve_it;
                
                if (sum[eh.idx()] == 1){
                mesh->set_color(*v_it, atcg::Mesh::Color(255, 0, 0));
                break;
                }
            }

        }

        // 
    }

    // This is run at the start of the program
    virtual void onAttach() override
    {
        const auto& window = atcg::Application::get()->getWindow();
        float aspect_ratio = (float)window->getWidth() / (float)window->getHeight();
        camera_controller  = std::make_shared<atcg::CameraController>(aspect_ratio);

        ear_mesh = atcg::IO::read_mesh("res/maxear.obj");
        ear_mesh->request_vertex_colors();
        for(auto vt: ear_mesh->vertices())
        {
            ear_mesh->set_color(vt, atcg::Mesh::Color(255));
        }

        atcg::normalize(ear_mesh);

        Eigen::SparseMatrix<int> M = compute_boundary_operator(ear_mesh);
        Eigen::VectorXi sum        = compute_boundary_edges(M);
        color_mesh(ear_mesh, sum);

        ear_mesh->uploadData();

        plane_mesh = atcg::IO::read_mesh("res/plane.obj");
        plane_mesh->request_vertex_colors();
        for(auto vt: plane_mesh->vertices())
        {
            plane_mesh->set_color(vt, atcg::Mesh::Color(255));
        }

        atcg::normalize(plane_mesh);
        plane_mesh->setScale(glm::vec3(1.5f));

        plane_mesh->uploadData();

        bunny_mesh = atcg::IO::read_mesh("res/bunny.obj");
        bunny_mesh->request_vertex_colors();
        for(auto vt: bunny_mesh->vertices())
        {
            bunny_mesh->set_color(vt, atcg::Mesh::Color(255));
        }

        atcg::normalize(bunny_mesh);

        Eigen::SparseMatrix<int> M_bunny = compute_boundary_operator(bunny_mesh);
        Eigen::VectorXi cut_faces        = compute_plane_cut(bunny_mesh, plane_normal);
        Eigen::VectorXi cut_edges        = detect_boundary(M_bunny, cut_faces);
        color_mesh(bunny_mesh, cut_edges);

        bunny_mesh->uploadData();
    }

    // This gets called each frame
    virtual void onUpdate(float delta_time) override
    {
        camera_controller->onUpdate(delta_time);

        atcg::Renderer::clear();

        if(show_ear)
        {
            if(ear_mesh && render_faces)
                atcg::Renderer::draw(ear_mesh, atcg::ShaderManager::getShader("base"), camera_controller->getCamera());

            if(ear_mesh && render_points)
                atcg::Renderer::drawPoints(ear_mesh,
                                           glm::vec3(0),
                                           atcg::ShaderManager::getShader("base"),
                                           camera_controller->getCamera());

            if(ear_mesh && render_edges)
                atcg::Renderer::drawLines(ear_mesh, glm::vec3(0), camera_controller->getCamera());
        }
        else
        {
            if(bunny_mesh && render_faces)
                atcg::Renderer::draw(bunny_mesh,
                                     atcg::ShaderManager::getShader("base"),
                                     camera_controller->getCamera());

            if(bunny_mesh && render_points)
                atcg::Renderer::drawPoints(bunny_mesh,
                                           glm::vec3(0),
                                           atcg::ShaderManager::getShader("base"),
                                           camera_controller->getCamera());

            if(bunny_mesh && render_edges)
                atcg::Renderer::drawLines(bunny_mesh, glm::vec3(0), camera_controller->getCamera());

            if(plane_mesh && show_plane)
                atcg::Renderer::draw(plane_mesh,
                                     atcg::ShaderManager::getShader("base"),
                                     camera_controller->getCamera());
        }
    }

    virtual void onImGuiRender() override
    {
        ImGui::BeginMainMenuBar();

        if(ImGui::BeginMenu("Rendering"))
        {
            ImGui::MenuItem("Show Render Settings", nullptr, &show_render_settings);
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Exercise"))
        {
            ImGui::MenuItem("Show Settings", nullptr, &show_settings);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();

        if(show_render_settings)
        {
            ImGui::Begin("Settings", &show_render_settings);

            ImGui::Checkbox("Render Faces", &render_faces);
            ImGui::Checkbox("Render Edges", &render_edges);
            ImGui::Checkbox("Render Points", &render_points);

            ImGui::End();
        }

        if(show_settings)
        {
            ImGui::Begin("Exercise Settings", &show_settings);

            if(show_ear)
            {
                if(ImGui::Button("Show Cut"))
                {
                    show_ear = false;
                }
            }
            else
            {
                ImGui::Checkbox("Show Plane", &show_plane);

                if(ImGui::DragFloat("Plane Angle", &angle, 0.01f, -3.14f, 3.14f))
                {
                    plane_normal = glm::vec3(sin(angle), cos(angle), 0);

                    plane_mesh->setRotation(glm::vec3(0, 0, 1.0f), -angle);

                    for(auto vt: bunny_mesh->vertices())
                    {
                        bunny_mesh->set_color(vt, atcg::Mesh::Color(255));
                    }

                    Eigen::SparseMatrix<int> M_bunny = compute_boundary_operator(bunny_mesh);
                    Eigen::VectorXi cut_faces        = compute_plane_cut(bunny_mesh, plane_normal);
                    Eigen::VectorXi cut_edges        = detect_boundary(M_bunny, cut_faces);
                    color_mesh(bunny_mesh, cut_edges);

                    bunny_mesh->uploadData();
                }

                if(ImGui::Button("Show Ear"))
                {
                    show_ear = true;
                }
            }

            ImGui::End();
        }
    }

    // This function is evaluated if an event (key, mouse, resize events, etc.) are triggered
    virtual void onEvent(atcg::Event& event) override { camera_controller->onEvent(event); }

private:
    std::shared_ptr<atcg::Mesh> ear_mesh;
    std::shared_ptr<atcg::Mesh> bunny_mesh;
    std::shared_ptr<atcg::Mesh> plane_mesh;
    std::shared_ptr<atcg::CameraController> camera_controller;

    bool show_render_settings = false;
    bool render_faces         = true;
    bool render_edges         = true;
    bool render_points        = false;

    bool show_settings = true;
    bool show_ear      = true;
    bool show_plane    = true;

    glm::vec3 plane_normal = glm::vec3(0, 1, 0);
    float angle            = 0.0f;
};

class Exercise04 : public atcg::Application
{
public:
    Exercise04() : atcg::Application() { pushLayer(new Exercise04Layer("Layer")); }

    ~Exercise04() {}
};

atcg::Application* atcg::createApplication()
{
    return new Exercise04;
}