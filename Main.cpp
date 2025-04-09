#include <opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
std::vector<cv::Point2f> imagePoints;
std::vector<cv::Point3f> objectPoints;
std::string path = std::filesystem::current_path().string();
std::vector<cv::Point3f> loadPLYVertices(const std::string& filename) {
    std::vector<cv::Point3f> vertices;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier " << filename << std::endl;
        return vertices;
    }

    std::string line;
    bool headerEnded = false;
    int vertexCount = 0;

    // Lire l'en-t�te
    while (std::getline(file, line)) {
        if (line == "end_header") {
            headerEnded = true;
            break;
        }
        if (line.substr(0, 14) == "element vertex") {
            sscanf(line.c_str(), "element vertex %d", &vertexCount);
        }
    }

    if (!headerEnded) {
        std::cerr << "Erreur : En-t�te du fichier .ply incorrect" << std::endl;
        return vertices;
    }

    // Lire les sommets
    for (int i = 0; i < vertexCount; ++i) {
        std::getline(file, line);
        float x, y, z;
        std::istringstream iss(line);
        iss >> x >> y >> z;
        vertices.emplace_back(x, y, z);
    }

    return vertices;
}


std::vector<cv::Point2f> imagePoints;

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        imagePoints.push_back(cv::Point2f((float)x, (float)y));
        std::cout << "Point ajout� : (" << x << ", " << y << ")" << std::endl;
    }
}


int main() {
    // Charger l'image
	string chemin = path + "baseColor.jpg";
    cv::Mat img = cv::imread(chemin);
    if (img.empty()) {
        std::cerr << "Erreur : Impossible de charger l'image." << std::endl;
        return -1;
    }

	string chemin2 = path + "point_cloud_1.ply";
    // Charger les sommets du mod�le 3D
    auto vertices = loadPLYVertices("chemin2");

    if (vertices.empty()) {
        std::cerr << "Erreur lors du chargement du mod�le 3D." << std::endl;
        return -1;
    }

    std::cout << "Mod�le charg�, nombre de sommets : " << vertices.size() << std::endl;
    
    // Charger l'image
    cv::Mat img = cv::imread(chemin);
    if (img.empty()) {
        std::cerr << "Erreur: Impossible de charger l'image." << std::endl;
        return -1;
    }

    // Cr�er une fen�tre
    const std::string windowName = "Cliquez pour s�lectionner des points";
    cv::namedWindow(windowName);
    cv::setMouseCallback(windowName, mouseCallback);

    while (true) {
        cv::Mat imgCopy = img.clone();

        // Dessiner tous les points cliqu�s
        for (size_t i = 0; i < imagePoints.size(); ++i) {
            cv::circle(imgCopy, imagePoints[i], 5, cv::Scalar(0, 255, 0), -1);
            cv::putText(imgCopy, std::to_string(i), imagePoints[i] + cv::Point2f(5, -5),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
        }

        cv::imshow(windowName, imgCopy);

        char key = (char)cv::waitKey(1);
        if (key == 27) { // ESC pour quitter
            break;
        }
    }

    // Afficher tous les points collect�s
    std::cout << "Points cliqu�s :" << std::endl;
    for (const auto& pt : imagePoints) {
        std::cout << pt << std::endl;
    }

    // Sauvegarder les points dans un fichier si tu veux
    std::ofstream file("points_image.txt");
    for (const auto& pt : imagePoints) {
        file << pt.x << " " << pt.y << std::endl;
    }
    file.close();

    return 0;
}