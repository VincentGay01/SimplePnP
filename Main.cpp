#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Variables globales
std::vector<cv::Point2f> imagePoints;
std::vector<cv::Point3f> objectPoints;
std::vector<cv::Point3f> allVertices;
bool waitingFor3DPoint = false;
int currentImagePointIndex = -1;
cv::Mat image;
cv::Mat model3DView;

// Structure pour stockage des états
struct AppState {
    bool imagePointSelected = false;
    cv::Point2f lastImagePoint;
    int lastSelectedVertex = -1;
};

AppState state;

// Fonction pour créer une visualisation simple du modèle 3D
cv::Mat create3DModelView(const std::vector<cv::Point3f>& vertices, int width, int height) {
    cv::Mat view = cv::Mat::zeros(height, width, CV_8UC3);

    // Trouver les limites du modèle
    cv::Point3f minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    cv::Point3f maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& v : vertices) {
        minBounds.x = std::min(minBounds.x, v.x);
        minBounds.y = std::min(minBounds.y, v.y);
        minBounds.z = std::min(minBounds.z, v.z);

        maxBounds.x = std::max(maxBounds.x, v.x);
        maxBounds.y = std::max(maxBounds.y, v.y);
        maxBounds.z = std::max(maxBounds.z, v.z);
    }

    // Calculer les facteurs d'échelle pour ajuster à la vue
    float rangeX = maxBounds.x - minBounds.x;
    float rangeY = maxBounds.y - minBounds.y;
    float rangeZ = maxBounds.z - minBounds.z;

    float scale = std::min(width / rangeX, height / rangeY) * 0.8f;

    // Dessiner chaque point
    for (size_t i = 0; i < vertices.size(); ++i) {
        int x = static_cast<int>((vertices[i].x - minBounds.x) * scale + width * 0.1);
        int y = static_cast<int>((vertices[i].y - minBounds.y) * scale + height * 0.1);

        // Couleur basée sur la coordonnée Z
        int colorZ = static_cast<int>(255 * (vertices[i].z - minBounds.z) / rangeZ);
        cv::circle(view, cv::Point(x, y), 2, cv::Scalar(colorZ, 255 - colorZ, 128), -1);

        // Ajouter un numéro pour chaque 10ème point pour aider à l'identification
        if (i % 10 == 0) {
            cv::putText(view, std::to_string(i), cv::Point(x + 3, y - 3),
                cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(200, 200, 200), 1);
        }
    }

    return view;
}

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

    // Lire l'en-tête
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
        std::cerr << "Erreur : En-tête du fichier .ply incorrect" << std::endl;
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

void imageMouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (!state.imagePointSelected) {
            state.lastImagePoint = cv::Point2f((float)x, (float)y);
            state.imagePointSelected = true;

            std::cout << "Point image sélectionné : (" << x << ", " << y << ")" << std::endl;
            std::cout << "Sélectionnez maintenant le point 3D correspondant" << std::endl;

            // Marquer le point temporairement
            cv::Mat imgCopy = image.clone();
            cv::circle(imgCopy, state.lastImagePoint, 5, cv::Scalar(0, 255, 255), -1);
            cv::imshow("Image", imgCopy);
        }
    }
}

void model3DMouseCallback(int event, int x, int y, int flags, void* userdata) {
    cv::Mat viewCopy = model3DView.clone();

    // Afficher le point le plus proche sous la souris
    if (event == cv::EVENT_MOUSEMOVE) {
        // Cette partie est plus complexe et nécessiterait de recalculer la position 3D à partir de x,y
        // Simplification: montrer un indicateur à la position actuelle
        cv::circle(viewCopy, cv::Point(x, y), 3, cv::Scalar(0, 255, 255), -1);
        cv::imshow("Modèle 3D", viewCopy);
    }

    // Sélectionner un point 3D
    if (event == cv::EVENT_LBUTTONDOWN && state.imagePointSelected) {
        // Demander le numéro du point 3D (simplification)
        std::cout << "Entrez l'index du point 3D: ";
        int idx;
        std::cin >> idx;

        if (idx >= 0 && idx < allVertices.size()) {
            // Ajouter les points aux correspondances
            imagePoints.push_back(state.lastImagePoint);
            objectPoints.push_back(allVertices[idx]);

            std::cout << "Correspondance ajoutée: Image(" << state.lastImagePoint.x << ", "
                << state.lastImagePoint.y << ") -> 3D("
                << allVertices[idx].x << ", " << allVertices[idx].y << ", "
                << allVertices[idx].z << ")" << std::endl;

            // Réinitialiser l'état
            state.imagePointSelected = false;

            // Mettre à jour les vues avec toutes les correspondances
            cv::Mat imgWithPoints = image.clone();
            for (size_t i = 0; i < imagePoints.size(); ++i) {
                cv::circle(imgWithPoints, imagePoints[i], 5, cv::Scalar(0, 255, 0), -1);
                cv::putText(imgWithPoints, std::to_string(i), imagePoints[i] + cv::Point2f(5, -5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
            }
            cv::imshow("Image", imgWithPoints);

            // Mettre à jour la vue du modèle 3D
            cv::Mat modelWithPoints = model3DView.clone();
            // Cette partie est simplifiée car nous ne recalculons pas les coordonnées écran
            cv::imshow("Modèle 3D", modelWithPoints);
        }
        else {
            std::cout << "Index de point 3D invalide!" << std::endl;
        }
    }
}

int main() {
    // Charger l'image
    image = cv::imread("D:/project/SimplePnP/baseColor.png");
    if (image.empty()) {
        std::cerr << "Erreur : Impossible de charger l'image." << std::endl;
        return -1;
    }

    // Charger les sommets du modèle 3D
    allVertices = loadPLYVertices("D:/project/SimplePnP/point_cloud_1.ply");

    if (allVertices.empty()) {
        std::cerr << "Erreur lors du chargement du modèle 3D." << std::endl;
        return -1;
    }

    std::cout << "Modèle chargé, nombre de sommets : " << allVertices.size() << std::endl;

    // Créer une visualisation du modèle 3D
    model3DView = create3DModelView(allVertices, image.cols, image.rows);

    // Créer les fenêtres
    cv::namedWindow("Image");
    cv::namedWindow("Modèle 3D");

    // Configurer les callbacks
    cv::setMouseCallback("Image", imageMouseCallback);
    cv::setMouseCallback("Modèle 3D", model3DMouseCallback);

    // Afficher les fenêtres initiales
    cv::imshow("Image", image);
    cv::imshow("Modèle 3D", model3DView);

    std::cout << "Instructions:" << std::endl;
    std::cout << "1. Cliquez sur un point dans l'image" << std::endl;
    std::cout << "2. Cliquez sur le point correspondant dans la vue 3D et entrez son index" << std::endl;
    std::cout << "3. Répétez pour au moins 4 points" << std::endl;
    std::cout << "4. Appuyez sur 'Esc' pour calculer la pose avec PnP" << std::endl;

    // Boucle principale
    while (true) {
        char key = (char)cv::waitKey(10);
        if (key == 27) { // ESC pour quitter
            break;
        }
    }

    // Vérifier si nous avons suffisamment de points
    if (imagePoints.size() < 4) {
        std::cerr << "Pas assez de correspondances pour PnP (minimum 4 requis). "
            << "Vous avez fourni " << imagePoints.size() << " points." << std::endl;
        return -1;
    }

    // Afficher tous les points collectés
    std::cout << "Correspondances établies:" << std::endl;
    for (size_t i = 0; i < imagePoints.size(); ++i) {
        std::cout << "Image point " << i << ": (" << imagePoints[i].x << ", " << imagePoints[i].y << ")"
            << " -> 3D point: (" << objectPoints[i].x << ", " << objectPoints[i].y
            << ", " << objectPoints[i].z << ")" << std::endl;
    }

    // Sauvegarder les points dans un fichier
    std::ofstream filePoints("points_correspondances.txt");
    for (size_t i = 0; i < imagePoints.size(); ++i) {
        filePoints << imagePoints[i].x << " " << imagePoints[i].y << " "
            << objectPoints[i].x << " " << objectPoints[i].y << " " << objectPoints[i].z << std::endl;
    }
    filePoints.close();

    // Lire les paramètres intrinsèques
    std::ifstream fileIntrinsics("D:/project/SimplePnP/intrinsics.json");
    if (!fileIntrinsics.is_open()) {
        std::cerr << "Erreur d'ouverture du fichier JSON." << std::endl;
        return -1;
    }

    json j;
    try {
        fileIntrinsics >> j;
    }
    catch (const json::parse_error& e) {
        std::cerr << "Erreur de parsing JSON : " << e.what() << std::endl;
        return 1;
    }

    // Extraire les paramètres intrinsèques
    double fx = j["fx"];
    double fy = j["fy"];
    double cx = j["cx"];
    double cy = j["cy"];
    int width = j["width"];
    int height = j["height"];

    cv::Mat rvec, tvec;
    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        fx, 0, cx,
        0, fy, cy,
        0, 0, 1);
    cv::Mat distCoeffs = cv::Mat::zeros(5, 1, CV_64F); // ou la vraie distortion si disponible

    bool success = cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec);

    if (success) {
        std::cout << "Rotation vector : " << rvec.t() << std::endl;
        std::cout << "Translation vector : " << tvec.t() << std::endl;

        // Sauvegarder les résultats
        json result;
        std::vector<double> rotation = { rvec.at<double>(0), rvec.at<double>(1), rvec.at<double>(2) };
        std::vector<double> translation = { tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2) };

        result["rotation"] = rotation;
        result["translation"] = translation;

        std::ofstream resultFile("pnp_result.json");
        resultFile << result.dump(4);
        resultFile.close();

        // Projeter les points 3D pour vérification
        std::vector<cv::Point2f> projectedPoints;
        cv::projectPoints(objectPoints, rvec, tvec, cameraMatrix, distCoeffs, projectedPoints);

        // Afficher l'erreur de reprojection
        double totalError = 0;
        for (size_t i = 0; i < projectedPoints.size(); ++i) {
            double error = cv::norm(imagePoints[i] - projectedPoints[i]);
            totalError += error;
            std::cout << "Point " << i << " - Erreur: " << error << " pixels" << std::endl;
        }
        std::cout << "Erreur moyenne de reprojection: " << totalError / projectedPoints.size() << " pixels" << std::endl;

        // Afficher l'image avec les points projetés pour vérification
        cv::Mat imgWithProjection = image.clone();
        for (size_t i = 0; i < imagePoints.size(); ++i) {
            // Point original en vert
            cv::circle(imgWithProjection, imagePoints[i], 5, cv::Scalar(0, 255, 0), -1);
            // Point projeté en rouge
            cv::circle(imgWithProjection, projectedPoints[i], 3, cv::Scalar(0, 0, 255), -1);
            // Ligne entre les deux
            cv::line(imgWithProjection, imagePoints[i], projectedPoints[i], cv::Scalar(255, 0, 0), 1);
        }

        cv::imshow("Vérification de reprojection", imgWithProjection);
        cv::waitKey(0);
    }
    else {
        std::cerr << "Échec de solvePnP." << std::endl;
    }

    return 0;
}