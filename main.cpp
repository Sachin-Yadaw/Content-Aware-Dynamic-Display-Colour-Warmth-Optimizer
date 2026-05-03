#include <windows.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

using namespace std;

vector<double> gaussianSolver(vector<vector<double>> A, vector<double> B) {
    int n = 3;
    for (int i = 0; i < n; i++) {
        int pivot = i;
        for (int j = i + 1; j < n; j++)
            if (abs(A[j][i]) > abs(A[pivot][i])) pivot = j;
        swap(A[i], A[pivot]); swap(B[i], B[pivot]);
        for (int k = i + 1; k < n; k++) {
            double factor = A[k][i] / A[i][i];
            B[k] -= factor * B[i];
            for (int j = i; j < n; j++) A[k][j] -= factor * A[i][j];
        }
    }
    vector<double> x(n);
    for (int i = n - 1; i >= 0; i--) {
        double sum = 0;
        for (int j = i + 1; j < n; j++) sum += A[i][j] * x[j];
        x[i] = (B[i] - sum) / A[i][i];
    }
    return x; 
}

double secantOptimizer(double energy) {
    auto f = [&](double x) { 
        double currentContrast = 10.0 * (1.0 - (x * 0.7)); 
        return currentContrast - 4.5; 
    };
    
    double x0 = 0.1, x1 = 0.2; 
    for (int i = 0; i < 5; i++) {
        double f0 = f(x0), f1 = f(x1);
        if (abs(f1 - f0) < 0.0001) break;
        double x_next = x1 - f1 * (x1 - x0) / (f1 - f0);
        x0 = x1; x1 = x_next;
    }
   
    double maxPossible = (energy / 255.0); 
    return max(0.0, min(maxPossible, x1));
}


class SplineSmoother {
    double start, end, t;
public:
    SplineSmoother() : start(0), end(0), t(1.0) {}
    void startTransition(double s, double e) { start = s; end = e; t = 0; }
    double getNext() {
        if (t >= 1.0) return end;
        t += 0.04; // Transition step
        double h01 = -2 * pow(t, 3) + 3 * pow(t, 2); // Hermite Spline
        return start + (end - start) * h01;
    }
    bool isTransitioning() { return t < 1.0; }
};

double getScreenEnergy() {
    HDC hdc = GetDC(NULL);
    int w = GetSystemMetrics(SM_CXSCREEN), h = GetSystemMetrics(SM_CYSCREEN);
    double sum = 0;
    // Nodes for Integration
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            COLORREF c = GetPixel(hdc, (w/10)*i, (h/10)*j);
            sum += GetBValue(c);
        }
    }
    ReleaseDC(NULL, hdc);
    return sum / 100.0;
}

void applyHardwareTint(double redVal, double greenVal, double blueVal) {
    WORD ramp[3][256];
    for (int i = 0; i < 256; i++) {
        ramp[0][i] = (WORD)(i * 256 * redVal);
        ramp[1][i] = (WORD)(i * 256 * greenVal);
        ramp[2][i] = (WORD)(i * 256 * blueVal);
    }
    HDC hdc = GetDC(NULL);
    SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);
}

int main() {
    cout << "OptiSight Final Engine: System-Wide Adaptive Tint Active" << endl;
    double currentRed = 1.0, currentGreen = 1.0, currentBlue = 1.0;
    SplineSmoother rSmooth, gSmooth, bSmooth;

    while (true) {
        double energy = getScreenEnergy();
        double optReduction = secantOptimizer(energy);


        vector<vector<double>> A = {{1,0,0},{0,1,0},{0,0,1}};
        vector<double> B = {1.0, 1.0 - (optReduction * 0.2), 1.0 - optReduction};
        
        vector<double> targets = gaussianSolver(A, B);

        rSmooth.startTransition(currentRed, targets[0]);
        gSmooth.startTransition(currentGreen, targets[1]);
        bSmooth.startTransition(currentBlue, targets[2]);

        while (rSmooth.isTransitioning()) {
            currentRed = rSmooth.getNext();
            currentGreen = gSmooth.getNext();
            currentBlue = bSmooth.getNext();
            applyHardwareTint(currentRed, currentGreen, currentBlue);
            Sleep(25);
        }
        cout << "Blue Energy: " << (int)energy << " | Optimal Blue Target: " << (int)(targets[2]*100) << "%" << endl;
        Sleep(2000);
    }
    return 0;
}
