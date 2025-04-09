#include <opencv2/opencv.hpp>
#include <opencv2/viz.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/viz/widgets.hpp>
#include <iostream>
#include <vector>
#include <string>

// Variables globales pour stocker les points
std::vector<cv::Point2f> imagePoints;
std::vector<cv::Point3f> objectPoints;
cv::Mat image;
std::string windowName = "S�lection des points sur l'image";

// Fonction callback pour les clics souris
void onMouseClick(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        cv::Point2f point(x, y);
        imagePoints.push_back(point);

        // Marquer le point sur l'image
        cv::circle(image, point, 5, cv::Scalar(0, 255, 0), -1);
        cv::putText(image, std::to_string(imagePoints.size()),
            cv::Point(x + 10, y - 10), cv::FONT_HERSHEY_SIMPLEX,
            0.5, cv::Scalar(0, 255, 0), 2);

        cv::imshow(windowName, image);
        std::cout << "Point 2D #" << imagePoints.size() << " s�lectionn�: (" << x << ", " << y << ")" << std::endl;
    }
}

int main() {
    // 1. Demander le chemin du fichier PLY
    std::string plyFilePath;
    std::cout << "Entrez le chemin du fichier PLY: ";
    std::cin >> plyFilePath;

    // 2. Charger le maillage 3D depuis le fichier PLY
    cv::viz::Mesh mesh;
    try {
        mesh = cv::viz::Mesh::load(plyFilePath);
        std::cout << "Maillage PLY charg� avec succ�s." << std::endl;
        std::cout << "Nombre de sommets: " << mesh.cloud.size() << std::endl;
    }
    catch (const cv::Exception& e) {
        std::cerr << "Erreur lors du chargement du fichier PLY: " << e.what() << std::endl;
        return -1;
    }

    // Extraire les sommets du maillage
    std::vector<cv::Point3f> meshVertices;
    for (int i = 0; i < mesh.cloud.rows; i++) {
        cv::Point3f vertex(
            mesh.cloud.at<cv::Vec3f>(i)[0],
            mesh.cloud.at<cv::Vec3f>(i)[1],
            mesh.cloud.at<cv::Vec3f>(i)[2]
        );
        meshVertices.push_back(vertex);
    }

    // 3. Demander le chemin de l'image
    std::string imagePath;
    std::cout << "Entrez le chemin de l'image: ";
    std::cin >> imagePath;

    // 4. Charger l'image
    image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Impossible de charger l'image!" << std::endl;
        return -1;
    }

    // Cr�er une copie de l'image originale pour la restauration apr�s chaque s�lection
    cv::Mat originalImage = image.clone();

    // 5. Afficher le maillage 3D avec VIZ
    cv::viz::Viz3d window3D("Maillage 3D");

    // Ajouter des axes de coordonn�es
    window3D.showWidget("Axes", cv::viz::WCoordinateSystem(1.0));

    // Cr�ation d'un widget pour afficher le maillage complet
    cv::viz::WMesh meshWidget(mesh);
    window3D.showWidget("Maillage", meshWidget);

    std::cout << "Visualisation 3D du maillage." << std::endl;
    std::cout << "Vous pouvez faire pivoter le mod�le avec la souris." << std::endl;
    std::cout << "Appuyez sur Q dans la fen�tre 3D pour continuer." << std::endl;
    window3D.spin();

    // 6. Permettre � l'utilisateur de s�lectionner des points 3D sp�cifiques
    std::cout << "\nS�lection des points 3D:" << std::endl;
    objectPoints.clear();

    // Limiter le nombre de sommets � afficher pour la s�lection
    int maxVerticesToDisplay = std::min(500, (int)meshVertices.size());
    int stepSize = meshVertices.size() / maxVerticesToDisplay;
    stepSize = stepSize < 1 ? 1 : stepSize;

    std::vector<cv::Point3f> sampledVertices;
    for (size_t i = 0; i < meshVertices.size(); i += stepSize) {
        sampledVertices.push_back(meshVertices[i]);
    }

    std::cout << "Nombre de sommets �chantillonn�s pour la s�lection: " << sampledVertices.size() << std::endl;

    bool selectionComplete = false;
    int selectedIndex = 0;

    // Cr�er un widget nuage de points pour les sommets �chantillonn�s
    cv::viz::WCloud cloudWidget(sampledVertices, cv::viz::Color::white());
    cloudWidget.setRenderingProperty(cv::viz::POINT_SIZE, 5);
    window3D.showWidget("SampledPoints", cloudWidget);

    while (!selectionComplete) {
        window3D.removeWidget("PointMarker");

        // Mettre en �vidence le point actuel
        cv::viz::WSphere currentPoint(sampledVertices[selectedIndex], 0.02, 10, cv::viz::Color::red());
        window3D.showWidget("PointMarker", currentPoint);
        window3D.spinOnce(1, true);

        std::cout << "Point 3D #" << (selectedIndex + 1) << " / " << sampledVertices.size() << ": "
            << sampledVertices[selectedIndex] << std::endl;
        std::cout << "Commandes: (s)�lectionner ce point, (n)ext point, (p)revious point, (q)uitter la s�lection: ";
        char response;
        std::cin >> response;

        switch (response) {
        case 's': case 'S':
            objectPoints.push_back(sampledVertices[selectedIndex]);
            std::cout << "Point 3D #" << objectPoints.size() << " s�lectionn�: "
                << sampledVertices[selectedIndex] << std::endl;
            break;
        case 'n': case 'N':
            selectedIndex = (selectedIndex + 1) % sampledVertices.size();
            break;
        case 'p': case 'P':
            selectedIndex = (selectedIndex - 1 + sampledVertices.size()) % sampledVertices.size();
            break;
        case 'q': case 'Q':
            if (objectPoints.size() >= 4) {
                selectionComplete = true;
            }
            else {
                std::cout << "Vous devez s�lectionner au moins 4 points pour l'estimation de pose!" << std::endl;
            }
            break;
        default:
            std::cout << "Commande non reconnue." << std::endl;
        }
    }

    window3D.close();

    // 7. Permettre � l'utilisateur de s�lectionner les points correspondants sur l'image
    std::cout << "\nS�lectionnez les points correspondants sur l'image (dans le m�me ordre que les points 3D)" << std::endl;
    std::cout << "Nombre de points � s�lectionner: " << objectPoints.size() << std::endl;

    // Configurer la fen�tre pour la s�lection des points sur l'image
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::resizeWindow(windowName, image.cols, image.rows);
    cv::setMouseCallback(windowName, onMouseClick);
    cv::imshow(windowName, image);

    // Attendre que l'utilisateur ait s�lectionn� assez de points
    while (imagePoints.size() < objectPoints.size()) {
        char key = cv::waitKey(10);
        if (key == 27) {  // Touche �chap pour quitter
            return 0;
        }
    }

    cv::destroyWindow(windowName);

    // 8. Calibration de la cam�ra (pour un cas r�el, ces param�tres devraient venir d'une calibration)
    // Ici nous utilisons des valeurs approximatives bas�es sur la taille de l'image
    double focalLength = image.cols;  // Une approximation raisonnable
    cv::Point2d principalPoint(image.cols / 2, image.rows / 2);

    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
        focalLength, 0, principalPoint.x,
        0, focalLength, principalPoint.y,
        0, 0, 1);

    cv::Mat distCoeffs = cv::Mat::zeros(5, 1, CV_64F);

    // 9. Estimer la pose de la cam�ra
    cv::Mat rvec, tvec;
    bool success = cv::solvePnP(objectPoints, imagePoints, cameraMatrix,
        distCoeffs, rvec, tvec);

    if (!success) {
        std::cerr << "�chec de l'estimation de la pose!" << std::endl;
        return -1;
    }

    // 10. Afficher les r�sultats
    std::cout << "\nR�sultats de l'estimation de pose:" << std::endl;
    std::cout << "Vecteur de rotation (rvec):" << std::endl << rvec << std::endl;
    std::cout << "Vecteur de translation (tvec):" << std::endl << tvec << std::endl;

    // Convertir le vecteur de rotation en matrice de rotation
    cv::Mat rotationMatrix;
    cv::Rodrigues(rvec, rotationMatrix);
    std::cout << "Matrice de rotation:" << std::endl << rotationMatrix << std::endl;

    // 11. Visualiser la pose sur l'image
    // Dessiner les axes 3D projet�s
    std::vector<cv::Point3f> axisPoints;
    float axisLength = 1.0;  // Ajustez cette valeur selon l'�chelle de votre mod�le
    axisPoints.push_back(cv::Point3f(0, 0, 0));
    axisPoints.push_back(cv::Point3f(axisLength, 0, 0));  // X: rouge
    axisPoints.push_back(cv::Point3f(0, axisLength, 0));  // Y: vert
    axisPoints.push_back(cv::Point3f(0, 0, axisLength));  // Z: bleu

    std::vector<cv::Point2f> projectedAxis;
    cv::projectPoints(axisPoints, rvec, tvec, cameraMatrix, distCoeffs, projectedAxis);

    // Dessiner les axes
    cv::line(image, projectedAxis[0], projectedAxis[1], cv::Scalar(0, 0, 255), 2);  // X: rouge
    cv::line(image, projectedAxis[0], projectedAxis[2], cv::Scalar(0, 255, 0), 2);  // Y: vert
    cv::line(image, projectedAxis[0], projectedAxis[3], cv::Scalar(255, 0, 0), 2);  // Z: bleu

    // Projeter une partie des sommets du maillage sur l'image
    // Limiter le nombre de points � projeter pour �viter de surcharger l'image
    std::vector<cv::Point3f> projectVertices;
    int projectionSample = std::max(1, (int)(meshVertices.size() / 500));
    for (size_t i = 0; i < meshVertices.size(); i += projectionSample) {
        projectVertices.push_back(meshVertices[i]);
    }

    std::vector<cv::Point2f> projectedMesh;
    cv::projectPoints(projectVertices, rvec, tvec, cameraMatrix, distCoeffs, projectedMesh);

    // Dessiner les points projet�s du maillage
    for (const auto& point : projectedMesh) {
        if (point.x >= 0 && point.x < image.cols && point.y >= 0 && point.y < image.rows) {
            cv::circle(image, point, 1, cv::Scalar(255, 255, 0), -1);
        }
    }

    // Dessiner les points de correspondance
    for (size_t i = 0; i < imagePoints.size(); i++) {
        cv::circle(image, imagePoints[i], 5, cv::Scalar(0, 255, 0), -1);
        cv::putText(image, std::to_string(i + 1),
            cv::Point(imagePoints[i].x + 10, imagePoints[i].y - 10),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
    }

    // 12. Afficher l'image finale avec la projection
    cv::namedWindow("R�sultat de l'estimation de pose", cv::WINDOW_NORMAL);
    cv::imshow("R�sultat de l'estimation de pose", image);
    std::cout << "Appuyez sur une touche pour terminer..." << std::endl;
    cv::waitKey(0);

    // Sauvegarder l'image r�sultante
    std::string outputPath = "pose_estimation_result.jpg";
    cv::imwrite(outputPath, image);
    std::cout << "Image de r�sultat sauvegard�e dans " << outputPath << std::endl;

    // 13. Sauvegarder les param�tres de la cam�ra dans un fichier
    std::string cameraParamsFile = "camera_params.yml";
    cv::FileStorage fs(cameraParamsFile, cv::FileStorage::WRITE);
    if (fs.isOpened()) {
        fs << "cameraMatrix" << cameraMatrix;
        fs << "distCoeffs" << distCoeffs;
        fs << "rotationVector" << rvec;
        fs << "translationVector" << tvec;
        fs << "rotationMatrix" << rotationMatrix;
        fs.release();
        std::cout << "Param�tres de la cam�ra sauvegard�s dans " << cameraParamsFile << std::endl;
    }
    else {
        std::cerr << "Impossible d'ouvrir le fichier pour sauvegarder les param�tres de la cam�ra." << std::endl;
    }

    return 0;
}