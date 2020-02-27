#pragma once
#include <Eigen/Dense>
#include <motion-generators/JointTrajectory.hpp>
#include <random>
#include <cmath>
#include <iostream>
#include <Eigen/StdVector>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/prettywriter.h>

template <typename T> class ModifiedFourierTrajectory : public JointTrajectory<T> {
private:
    T global_pulsation;
    size_t fourier_coeff_number;
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> fourier_coeff_a, fourier_coeff_b;
    Eigen::Matrix<T, Eigen::Dynamic, 6> poly_coeff;
    Eigen::Matrix<T, Eigen::Dynamic, 1> qinit;

    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<T> real_dist;
    static constexpr double twopi = 2 * M_PI;
    T tf;
    size_t number_of_parameters;
public:
    ModifiedFourierTrajectory(const ModifiedFourierTrajectory<T> &copy) : JointTrajectory<T>(copy) {
        global_pulsation = copy.global_pulsation;
        fourier_coeff_number = copy.fourier_coeff_number;
        fourier_coeff_a = copy.fourier_coeff_a;
        fourier_coeff_b = copy.fourier_coeff_b;
        poly_coeff = copy.poly_coeff;
        qinit = copy.qinit;
        gen = copy.gen;
        real_dist = copy.real_dist;
        tf = copy.tf;
        number_of_parameters = copy.number_of_parameters;
    }
    ModifiedFourierTrajectory() : JointTrajectory<T>() {
        gen = std::mt19937(rd());
        real_dist = std::uniform_real_distribution<T>(0, std::nextafter(1, std::numeric_limits<T>::max()));
    }
    ModifiedFourierTrajectory(size_t dof) : ModifiedFourierTrajectory<T>(dof, 6) {}
    ModifiedFourierTrajectory(size_t dof, size_t coeff_num) : JointTrajectory<T>(dof) {
        gen = std::mt19937(rd());
        real_dist = std::uniform_real_distribution<T>(0, std::nextafter(1, std::numeric_limits<T>::max()));
        global_pulsation = 1.0;
        fourier_coeff_number = coeff_num;
        fourier_coeff_a.resize(dof, fourier_coeff_number);
        fourier_coeff_b.resize(dof, fourier_coeff_number);
        poly_coeff.resize(dof, 6);
        // std::cout << poly_coeff.rows() << "," << poly_coeff.cols() << std::endl;
        // poly_coeff.Random();
        // std::cout << "D3" << std::endl;
        for (size_t i = 0; i < dof; i++) {
            for (size_t j = 0; j < fourier_coeff_number; j++) {
                fourier_coeff_a(i, j) = real_dist(gen);
                fourier_coeff_b(i, j) = real_dist(gen);
            }
        }
        // std::cout << "D4" << std::endl;
        tf = twopi / global_pulsation;
        qinit = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(dof);
        // std::cout << "D5" << std::endl;
        convert_to_coeffcients();
        number_of_parameters = fourier_coeff_number * dof * 2 + 1;
        this->runnable = true;
        // std::cout << "D6" << std::endl;
    }

    ~ModifiedFourierTrajectory() override {}

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

    void convert_to_coeffcients() {
        poly_coeff.setZero();
        poly_coeff.col(0) = qinit;
        tf = twopi / global_pulsation;
        for (size_t i = 0; i < fourier_coeff_number; i++) {
            T j = i + 1;

            poly_coeff.col(0) += fourier_coeff_b.col(i) / (global_pulsation * (i + 1));
            poly_coeff.col(1) += -fourier_coeff_a.col(i);
            poly_coeff.col(2) += -(global_pulsation / 2) * (fourier_coeff_b.col(i) * (i + 1));
            poly_coeff.col(3) += (-(60 / j) * fourier_coeff_b.col(i)
                                  + (60 / j) * fourier_coeff_b.col(i) * cos(j * tf * global_pulsation)
                                  - (60 / j) * fourier_coeff_a.col(i) * sin(j * tf * global_pulsation)
                                  + (9 * j) * fourier_coeff_b.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2)
                                  + 36 * fourier_coeff_a.col(i) * tf * global_pulsation
                                  - (3 * j) * fourier_coeff_b.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2) * cos(j * tf * global_pulsation)
                                  + (3 * j) * fourier_coeff_a.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2) * sin(j * tf * global_pulsation)
                                  + 24 * fourier_coeff_a.col(i) * tf * global_pulsation * cos(j * tf * global_pulsation)
                                  + 24 * fourier_coeff_b.col(i) * tf * global_pulsation * sin(j * tf * global_pulsation)) / (6 * std::pow(tf, 3) * global_pulsation);

            poly_coeff.col(4) += -(-(60 / j) * fourier_coeff_b.col(i)
                                   + (60 / j) * fourier_coeff_b.col(i) * cos(j * tf * global_pulsation)
                                   - (60 / j) * fourier_coeff_a.col(i) * sin(j * tf * global_pulsation)
                                   + (6 * j) * fourier_coeff_b.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2)
                                   + 32 * fourier_coeff_a.col(i) * tf * global_pulsation
                                   - (4 * j) * fourier_coeff_b.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2) * cos(j * tf * global_pulsation)
                                   + (4 * j) * fourier_coeff_a.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2) * sin(j * tf * global_pulsation)
                                   + 28 * fourier_coeff_a.col(i) * tf * global_pulsation * cos(j * tf * global_pulsation)
                                   + 28 * fourier_coeff_b.col(i) * tf * global_pulsation * sin(j * tf * global_pulsation)) / (4 * std::pow(tf, 4) * global_pulsation);

            poly_coeff.col(5) += (-(12 / j) * fourier_coeff_b.col(i)
                                  + (12 / j) * fourier_coeff_b.col(i) * cos(j * tf * global_pulsation)
                                  - (12 / j) * fourier_coeff_a.col(i) * sin(j * tf * global_pulsation)
                                  + j * fourier_coeff_b.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2)
                                  + 6 * fourier_coeff_a.col(i) * tf * global_pulsation
                                  - j * fourier_coeff_b.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2) * cos(j * tf * global_pulsation)
                                  + j * fourier_coeff_a.col(i) * std::pow(tf, 2) * std::pow(global_pulsation, 2) * sin(j * tf * global_pulsation)
                                  + 6 * fourier_coeff_a.col(i) * tf * global_pulsation * cos(j * tf * global_pulsation)
                                  + 6 * fourier_coeff_b.col(i) * tf * global_pulsation * sin(j * tf * global_pulsation)) / (2 * std::pow(tf, 5) * global_pulsation);
        }
    }
    void update(double time = 0) override {
        assert(this->runnable);
        this->position.setZero();
        for (size_t j = 1; j <= fourier_coeff_number; j++) {
            this->position += (fourier_coeff_a.col(j - 1) / (global_pulsation * j)) * std::sin(global_pulsation * j * time) -
                              (fourier_coeff_b.col(j - 1) / (global_pulsation * j)) * std::cos(global_pulsation * j * time);
        }
        for (size_t k = 0; k < 6; k++) {
            this->position += poly_coeff.col(k) * std::pow(time - (std::ceil(time / tf) - 1) * tf, k);
        }
        this->velocity.setZero();
        for (size_t i = 0; i < this->dof; i++) {
            for (size_t j = 1; j <= fourier_coeff_number; j++) {
                this->velocity[i] += (fourier_coeff_a(i, j - 1)) * std::cos(global_pulsation * j * time) +
                                     (fourier_coeff_b(i, j - 1)) * std::sin(global_pulsation * j * time);
            }
        }
        for (size_t k = 1; k < 6; k++) {
            this->velocity += k * poly_coeff.col(k) * std::pow(time - (std::ceil(time / tf) - 1) * tf, k - 1);
        }
        this->acceleration.setZero();
        for (size_t i = 0; i < this->dof; i++) {
            for (size_t j = 1; j <= fourier_coeff_number; j++) {
                this->acceleration[i] += -(fourier_coeff_a(i, j - 1) * (global_pulsation * j)) * std::sin(global_pulsation * j * time) +
                                         (fourier_coeff_b(i, j - 1) * (global_pulsation * j)) * std::cos(global_pulsation * j * time);
            }
        }
        for (size_t k = 2; k < 6; k++) {
            this->acceleration += (k - 1) * k * poly_coeff.col(k) * std::pow(time - (std::ceil(time / tf) - 1) * tf, k - 2);
        }
    }
    void setGlobalPulsation(T global_pulse) {
        global_pulsation = global_pulse;
        tf = twopi / global_pulsation;
    }
    void setCoefficentNumbers(size_t fourier_coeff_number) {
        this->fourier_coeff_number = fourier_coeff_number;
    }

    size_t getFourierCoefficientNumber() {
        return this->fourier_coeff_number;
    }
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> getFourierCoefficientsA() {
        return fourier_coeff_a;
    }
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> getFourierCoefficientsB() {
        return fourier_coeff_b;
    }

    void getParameters(Eigen::Matrix<T, Eigen::Dynamic, 1> &params) {
        params(0) = global_pulsation;
        params.segment(1, fourier_coeff_a.rows() * fourier_coeff_a.cols()) = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1> >(fourier_coeff_a.data(), fourier_coeff_a.cols() * fourier_coeff_a.rows());
        params.segment(1 + fourier_coeff_a.rows() * fourier_coeff_a.cols(), fourier_coeff_b.rows() * fourier_coeff_b.cols()) = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1> >(fourier_coeff_b.data(), fourier_coeff_b.cols() * fourier_coeff_b.rows());
    }

    void setFromParameters(Eigen::Matrix<T, Eigen::Dynamic, 1> params) {
        global_pulsation = params(0);
        fourier_coeff_a = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> >(params.segment(1, fourier_coeff_a.rows() * fourier_coeff_a.cols()).data(), fourier_coeff_a.rows(), fourier_coeff_a.cols());
        fourier_coeff_b = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> >(params.segment(1 + fourier_coeff_a.rows() * fourier_coeff_a.cols(), fourier_coeff_b.rows() * fourier_coeff_b.cols()).data(), fourier_coeff_b.rows(), fourier_coeff_b.cols());
        convert_to_coeffcients();
    }

    size_t getNumberOfParameters() {
        return number_of_parameters;
    }

    void setQinit(Eigen::Matrix<T, Eigen::Dynamic, 1> &qinit) {
        this->qinit = qinit;
    }

    void loadFromJSON(std::string filename) override {
        JointTrajectory<T>::loadFromJSON(filename);
        // global_pulsation = T(this->json["frequency"].number_value());
        setGlobalPulsation((this->json["frequency"].number_value()));
        this->dof = this->json["dof"].int_value();
        qinit = Eigen::Matrix<T, Eigen::Dynamic, 1>::Zero(this->dof);
        if (!this->json["qinit"].is_null()) {
            for (size_t i = 0; i < this->dof; i++) {
                qinit(i) = T(this->json["qinit"][i].number_value());
            }
        }
        this->JointTrajectory<T>::setDof(this->dof);
        fourier_coeff_number = this->json["coefficient_number"].int_value();
        fourier_coeff_a.resize(this->dof, fourier_coeff_number);
        fourier_coeff_b.resize(this->dof, fourier_coeff_number);
        for (int j = 0; j < this->dof; j++) {
            for (int i = 0; i < fourier_coeff_number; i++) {
                fourier_coeff_a(j, i) = T(this->json["coefficient_a"][i + j * fourier_coeff_number].number_value());
                fourier_coeff_b(j, i) = T(this->json["coefficient_b"][i + j * fourier_coeff_number ].number_value());
            }
        }
        poly_coeff.resize(this->dof, 6);
        convert_to_coeffcients();
        number_of_parameters = fourier_coeff_number * this->dof * 2 + 1;
        this->runnable = true;
    }

    void saveToJSON(std::string filename) {
        rapidjson::Document d;
        d.SetObject();
        rapidjson::Document::AllocatorType &allocator = d.GetAllocator();
        rapidjson::Value global_pulse(global_pulsation);
        rapidjson::Value fourier_coeff_num(fourier_coeff_number);
        rapidjson::Value dof(this->dof);
        d.AddMember("dof", dof, allocator);
        d.AddMember("frequency", global_pulse, allocator);
        d.AddMember("coefficient_number", fourier_coeff_num, allocator);
        rapidjson::Value a(rapidjson::kArrayType);
        rapidjson::Value b(rapidjson::kArrayType);
        for (int j = 0; j < this->dof; j++) {
            for (int i = 0; i < fourier_coeff_number; i++) {
                a.PushBack(fourier_coeff_a(j, i), allocator);
                b.PushBack(fourier_coeff_b(j, i), allocator);
            }
        }
        d.AddMember("coefficient_a", a, allocator);
        d.AddMember("coefficient_b", b, allocator);

        rapidjson::Value startQ(rapidjson::kArrayType);
        for (size_t i2 = 0; i2 < this->dof; i2++) {
            startQ.PushBack(qinit[i2], allocator);
        }
        d.AddMember("qinit", startQ, allocator);
        std::ofstream ofs(filename);
        rapidjson::OStreamWrapper osw(ofs);
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
        writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
        d.Accept(writer);
        ofs.flush();
        ofs.close();
    }

    void display() {
        std::cout << "PJT\n";
        std::cout << "Global Pulsation:" << global_pulsation << "\n";
        std::cout << "coefficients numbers";
        std::cout << fourier_coeff_number;
        std::cout << "\n";
        std::cout << "coefficients a:";
        for (size_t i = 0; i < this->dof; i++) {
            std::cout << fourier_coeff_a << "\n ";
        }
        std::cout << "\n";
        std::cout << "coefficients b:";
        for (size_t i = 0; i < this->dof; i++) {
            std::cout << fourier_coeff_b << "\n ";
        }
        std::cout << "\n";
        std::cout << "poly_coeffs: " << poly_coeff << "\n";
    }
    T getPeriodLength() override {
        return tf;
    }
};
