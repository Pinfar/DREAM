/**
 * Implementation of the Dreicer runaway rate neural network published in
 *
 *   Hesslow et al (2019), Journal of Plasma Physics 86 (6) 475850601
 *   https://doi.org/10.1017/S0022377819000874
 *
 * This implementation is based on the original Matlab implementation of the
 * neural network: https://github.com/unnerfelt/dreicer-nn
 */

#include "DREAM/Equations/DreicerNeuralNetwork.hpp"


using namespace DREAM;

/**
 * DATA FOR NEURAL NETWORK
 */
// Input/output normalization
const real_t DreicerNeuralNetwork::input_std[8]   = {4.96121897262, 5.58308339503, 0.195384621847, 4.98281996514, 2.45185472351, 0.304090526425, 0.0345813696948, 2.28862902169};
const real_t DreicerNeuralNetwork::input_mean[8]  = {7.1561340244, 9.20374749346, 0.105979895644, 7.08910012241, 47.8203654981, 0.536347788327, 0.0699809427188, -9.4087288982};
const real_t DreicerNeuralNetwork::output_std[1]  = {18.5581667797};
const real_t DreicerNeuralNetwork::output_mean[1] = {-23.1854797433};

// Bias vectors
const real_t DreicerNeuralNetwork::b5[1] = {-0.418279};
const real_t DreicerNeuralNetwork::b4[20] = {-0.0446875, -0.424694, -0.227015, 0.37404, 0.733276, -0.110852, -0.711429, 1.26621, 0.896138, 0.250037, 0.255551, 0.256522, -0.0361706, 0.0528172, 0.319298, -0.279135, 0.26757, 0.496949, -0.769363, 0.842001};
const real_t DreicerNeuralNetwork::b3[20] = {0.128172, -0.683651, -0.516939, 0.745595, -0.488622, 0.131711, -0.517039, 0.469413, -0.199541, -0.587764, -0.684157, -0.182467, -0.370432, 0.244092, -0.582557, 0.70217, -0.511129, 0.513963, 0.305889, 0.502879};
const real_t DreicerNeuralNetwork::b2[20] = {0.173745, 0.61208, 0.343965, -0.13089, 0.517653, 0.753334, 0.380617, -0.216445, 0.500696, -0.138661, 0.306767, -0.742229, -0.837064, 0.21742, 0.27826, -0.693451, 0.555142, 0.428025, -0.0885804, 0.090892};
const real_t DreicerNeuralNetwork::b1[20] = {0.922809, -0.633332, 0.956879, 0.567717, 1.11568, 0.633259, 0.714246, 0.426098, 0.575361, 0.577878, 0.936333, -0.6446, -0.448202, 0.500627, -0.322209, -0.303171, -0.681062, -0.219382, 0.340537, 0.526125};

// Weight matrices
const real_t DreicerNeuralNetwork::W5[1*20]  = {0.337752, 0.374084, -0.593294, -0.361416, -0.155218, 0.464647, 0.665467, -0.265803, -0.202628, -0.215852, 0.240948, -0.281392, -0.315222, -0.565223, -0.100042, -0.642941, -0.290093, -0.543789, 0.184368, 0.528849};
const real_t DreicerNeuralNetwork::W4[20*20] = {0.368177, 0.184829, -0.560227, -0.115185, -0.462034, 0.164514, -0.120286, 0.536835, -0.147727, -0.376015, -1.26467, -0.0296414, 0.772014, -0.0545817, -0.325723, 0.50935, 0.244991, 0.999156, 0.272433, 0.311871, -1.36548, 0.576593, 2.61011, 0.0431694, 1.0318, -1.63172, 0.765699, 0.210103, 0.0680111, 1.75889, 2.08005, -1.06198, -0.763734, -0.519196, -0.161627, -1.81337, 1.07309, -0.799507, -1.54005, -2.02158, 1.04826, -1.12259, -4.59455, 0.528969, -2.1872, 2.189, -0.110963, -0.292242, -1.0474, -2.98598, -2.04858, -0.798689, 0.299229, 0.357661, -1.64253, 3.03271, -2.50765, 1.71197, 1.14073, 1.48026, 0.0402462, 0.425129, 0.946804, 0.0814686, 0.0777344, 0.135154, -0.11875, 0.534286, 0.593187, 0.258756, 0.128252, 0.21382, 0.213534, -0.277574, -0.134206, -0.34152, 0.4438, 0.420038, -0.0809263, -0.330237, -0.080972, -0.224582, -0.0751428, 0.499232, 0.320622, -0.0123093, -0.900114, -0.161122, -0.169943, -0.319601, 0.0185757, 0.620744, -0.109781, -0.171143, 0.0898834, 0.338286, -0.0195254, -0.0111848, 0.276347, 0.587076, -0.781061, 1.21144, 0.810033, -0.239123, 0.769574, -0.838605, 0.291669, -0.515193, 0.712369, 0.734934, 0.69138, 0.999578, -0.223755, -0.676951, 0.690448, -0.767345, 0.694129, -0.682642, -0.374497, -0.734718, -1.42813, 1.30852, 2.35719, -0.398603, 1.28252, -2.39597, 0.409505, -0.218005, 0.393011, 1.92731, 1.86415, 1.04498, -1.06397, -0.911991, 0.757048, -2.59454, 1.71343, -1.33633, -1.16576, -2.17705, -0.0913602, -0.280092, -0.0143629, 0.774266, 0.404369, -0.123209, 0.108059, -0.0572768, -0.0634108, -0.66617, -0.00637025, -0.218143, -0.498515, 0.165459, -0.208131, -0.423798, 0.0144911, 0.3857, 0.104456, 0.565487, 0.0461368, -0.261179, 0.413426, -0.151523, -0.286866, 0.0201091, -0.897346, 0.409745, 0.27003, 0.106706, 0.225012, 0.285906, 0.204303, -0.17295, -0.216951, 0.220167, 0.0588716, 0.260261, 0.153343, 0.747052, -0.149143, 0.119632, 0.405423, -0.0644823, 0.508275, 0.0121008, 0.532532, 0.295206, -0.580885, 0.156933, 0.764086, -0.508776, 0.217802, -0.417704, 0.336616, -0.632041, 0.382293, 0.334995, 0.0890387, -0.166758, 0.0995101, -0.150017, 1.13488, 0.339228, 0.32899, 0.113807, -0.502745, 0.733366, -0.444251, 0.240784, 0.485941, 0.344276, 0.0354621, -0.156637, -0.00189884, -0.646388, 0.362274, -0.18258, 0.28175, -0.579672, 0.684206, -1.05341, -0.376836, 0.36301, -0.348347, 0.161115, -0.997939, 0.55223, -0.546627, -0.516084, -0.171914, -0.25354, -0.0174415, 0.724658, -0.3405, 0.0376426, -0.296512, 0.275121, 1.01707, 0.274907, 0.370442, -0.647735, -0.565447, -0.206517, -0.604842, 0.377543, -0.502339, 0.468982, -0.698431, -0.330499, -0.674239, -0.629746, -0.0289828, 0.819989, -0.440076, 0.268644, -0.631439, 0.314779, -0.0980648, 0.722044, 1.00773, -0.534754, -1.30016, 0.0768753, -1.07397, 0.765166, -0.104199, 0.570107, -0.603374, -1.16858, -1.19225, -1.38258, 0.744431, 0.46268, -0.943338, 1.27448, -1.08909, 1.48373, 0.443966, 1.3101, -0.0513708, 0.547224, 0.27016, -0.232318, 0.412965, -0.0579404, -0.376652, 0.178979, -0.81692, -0.292535, -0.317801, 0.280347, -1.42055, 1.04151, 0.155321, 0.196793, -0.37807, 0.251805, 0.247316, 0.294691, -1.86097, 0.535284, 1.86396, -0.0981481, 1.30908, -1.92795, 0.498077, -0.604332, 0.885834, 1.38406, 2.42445, 0.776068, -1.35035, -1.03209, 0.402718, -1.70537, 1.46804, -1.25604, -0.903853, -2.61665, 0.521576, -0.749999, 0.0745336, 0.154967, -0.339394, 0.328266, -0.732915, 0.5951, -0.269994, -0.00260376, -0.421335, -0.00287019, -0.191003, 0.54379, -0.278002, 0.251649, -0.275407, 0.141304, 0.255848, 0.173681, -0.910606, 1.13189, 3.562, 0.720915, 1.6599, -1.60717, 0.346719, 0.00660793, 0.813425, 1.97005, 2.49318, 0.803947, -0.749228, -0.491959, 1.03428, -1.89416, 1.73961, -1.9441, -0.379344, -1.57481, -0.354355, -0.194127, 0.132712, 0.0681167, 0.375614, 0.00747904, 1.01424, -0.269529, 0.502072, 0.204283, -0.118132, -0.630261, 0.0493873, -0.393316, 0.0495943, -0.471491, 0.297736, 0.814637, -0.23619, -0.0360835, 0.815003, -0.437042, -3.48827, 0.337271, -1.47166, 1.76608, 0.091413, 0.119616, -0.315356, -2.59521, -1.80075, -0.534502, 0.440897, 0.224883, -0.989074, 2.6667, -1.34834, 0.91139, 0.977069, 1.5336};
const real_t DreicerNeuralNetwork::W3[20*20] = {0.798648, -0.590286, 0.161605, -0.222502, 1.48792, -5.17583, -0.782166, -0.0362775, -0.295284, -0.631199, -0.709789, 0.0869203, -0.487162, -0.264981, 1.64455, -0.402963, -0.889236, -1.31054, -0.117374, -0.12159, -0.271328, -0.142595, -0.299367, 0.161558, 0.0620801, 0.244612, -0.170388, 0.153538, -0.457558, 0.533522, 0.0670842, 0.368609, 0.611192, -0.392684, -0.481635, 0.225143, 0.221331, -0.0108845, -0.200587, -0.0354804, -0.51515, -0.311638, -0.397901, 1.34474, -0.440452, 1.61427, -0.279103, 0.896592, -1.51599, 0.771818, -1.02479, 1.63511, 0.897754, 0.812751, -1.16295, 1.01478, -0.303355, 1.11423, 0.662333, -0.378256, -0.209699, 0.693754, 0.780732, -0.014095, 0.149367, 0.987456, -0.0496421, -0.0203096, 0.152974, 0.329064, 0.0436989, -0.361219, 0.0474346, -0.114737, 0.635192, -0.232297, -0.151619, -0.0548446, -0.12411, 0.461586, -0.162708, 0.37287, -0.25429, 0.119234, -1.51458, 1.0017, 0.719026, 0.408186, 0.788835, -0.799241, -0.24473, -0.664403, 1.32701, 0.893946, 0.0731557, 0.149816, -1.30953, 0.0420132, 0.575893, -0.0605411, 0.487192, 0.364639, 0.161652, -0.161719, 0.871022, -8.28656, 2.02827, 0.820919, 0.892192, -0.9852, -0.836436, 1.10592, -0.572246, -1.15889, 2.25593, -0.864246, -1.04071, -2.26616, -0.00314888, 0.630925, 0.287937, -0.0990251, -0.575313, 0.0259689, 0.0172405, 0.381092, -0.33231, 0.0615488, -0.169635, -0.206418, 0.00952784, -0.495152, 0.336105, 0.309371, -0.250035, 0.261739, 0.00439386, 0.0414084, -0.127834, -0.299852, 0.201558, -0.413254, 0.302251, -0.602931, 0.517395, 0.270802, 0.771004, 0.0933618, -0.246283, 0.386088, -0.124479, 0.144267, -0.284518, 0.128103, -0.125892, -0.291106, -0.769992, 0.0520541, -0.465636, 0.179297, 0.394295, -0.121547, -0.694481, -0.624935, -0.272448, -0.112072, 0.551507, 0.0401361, -0.0859958, 0.206528, -0.0832871, 0.0623015, 0.352137, 0.392531, -0.0748858, 0.177483, 0.327235, 0.115221, 0.259011, -0.495822, -0.753847, 0.0625514, 0.855396, 0.641491, 0.349213, 1.14561, -0.279842, 0.605178, -1.45917, 0.62557, -0.0161334, 2.11226, 0.204022, 0.5356, -0.906271, 0.933325, -0.624437, 0.370789, 0.464406, -0.11913, -0.731539, -0.634346, -0.374871, -0.0128045, -0.586763, 0.713175, 0.0406906, 0.913233, -1.27922, 0.178658, -0.0311367, 1.29264, 0.77938, 0.946126, -0.383231, 0.570816, -0.694262, 0.346754, 0.606461, -0.206266, -0.119107, -0.845374, -0.758508, -0.0598682, 0.0835409, 0.356316, 0.731314, -0.0835606, -1.10308, -0.514721, -0.0188483, 0.192887, -0.267396, 0.444204, 0.536897, -0.530731, -0.00205339, -0.0394782, 0.335859, 0.667388, 0.784417, -0.00391535, -0.450046, -0.542449, 0.077779, 0.371569, -0.107345, 0.00898499, -0.0925234, -0.275488, 0.396516, -0.27029, -0.00291379, -0.325925, 0.544039, -0.433025, 0.117356, -0.389011, -0.366878, 0.271807, -0.119205, 0.0378652, 0.662398, -0.155141, 0.337116, -0.0292891, -0.609813, -0.130105, -0.174704, 0.991414, 0.128244, 0.493108, -0.587536, -0.497031, 0.0968684, 0.268073, 0.164221, 0.117763, -0.0338622, 0.440523, -0.178962, -0.723967, -0.346912, 0.372762, 0.429524, -0.694208, -1.8616, 0.807806, -0.263004, -0.0202029, -0.589462, -0.649537, 0.395133, 0.581977, 0.470828, -0.151483, 0.271875, 0.231035, -0.0284809, 0.117936, 1.08198, 0.643595, 1.72735, -0.181353, 1.98125, -0.614522, -0.811515, -0.939671, 1.32279, 0.116295, 0.321026, -0.910552, -1.14778, -2.06083, 0.960121, -1.01186, 1.59585, -1.25784, -1.61333, 0.725057, -0.669374, -0.381854, 0.441582, 0.135991, 0.351911, 0.221303, -0.0537429, 0.547374, -0.952825, 0.819035, -0.247236, 1.34068, 0.537677, 0.464671, -0.928678, 0.0336602, -0.0418462, 0.00857665, 0.276629, -0.40026, 0.0364462, 0.325902, 0.399108, -0.251899, 0.381353, -0.819704, -0.402231, -0.325022, 0.904204, -0.766521, 0.157763, -1.20861, -0.601639, -0.407524, 0.696919, -0.535781, -0.403788, -0.353289, -0.368483, 0.544192, 1.14959, 0.175075, -0.0465164, -0.533736, -0.110547, -0.953343, 0.222638, -0.454097, 0.653319, -0.936125, 0.399231, -1.2968, -0.07813, -0.854401, 0.243319, -0.499674, 0.728109, -0.38491, -0.147376, -0.0666994, 0.342666, 0.587893, 1.10022, -0.23209, 0.395967, -0.71717, 0.180347, -0.812377, 0.522663, -0.340293, 0.513623, -0.862296, -0.873915, -0.837709, 1.0925, -0.322401, 0.292371, -0.513859, -0.131478, 0.37165};
const real_t DreicerNeuralNetwork::W2[20*20] = {-0.58635, -0.438672, -0.60388, -0.561489, -0.364413, 0.0203549, -0.159576, -0.112207, -0.3403, 0.00224112, -0.453241, -0.103128, -0.356028, 0.0308356, 0.0334945, -0.194722, -0.0678543, 0.487254, 0.233383, 0.728713, -0.422592, 0.0521457, 0.0376996, -0.23567, 0.00307703, 0.150821, -0.395477, 0.157741, -0.158223, 0.023745, 0.059298, -0.0128884, 0.0485015, 0.263134, 0.156603, -0.195811, 0.152993, 0.268613, 0.0971745, -0.203781, -0.10259, 0.297871, 0.29695, -0.000862472, -0.0799795, -0.140429, -0.451322, 0.218621, -0.110491, 0.0777079, -0.354804, 0.194333, 0.463175, -0.374828, 0.0936719, -0.177957, 0.0195798, 0.20257, -0.166048, -0.525594, 0.191137, -0.268088, 0.53645, -0.0632909, -0.461556, 0.331722, 0.174281, -0.221875, -0.062957, -0.0766876, -0.0595797, 0.438884, -0.19192, 0.238288, -0.612382, 0.459069, -0.307996, -0.704276, -0.159086, 0.295316, -0.11715, 0.369524, -0.183779, -0.101454, 0.0506903, -0.186628, -0.210327, 0.00719873, -0.640536, 0.293778, -0.123577, -0.294118, 0.623791, -0.301824, -0.14863, -0.366614, 0.211338, -0.466637, -0.345165, 0.197877, 0.247098, -0.0310993, -0.0253376, 0.762747, 1.67565, 0.243475, 0.0943831, 1.05864, 0.476329, 0.231629, 0.571226, -0.363462, -0.600402, 0.544163, 0.164779, -0.563833, -3.52757, 0.0706474, -0.283659, 0.309283, -0.564489, 0.522382, 0.138314, 0.270094, -0.110804, 0.118243, 0.00306089, -0.347042, 0.245692, 0.286423, 0.297822, -0.193218, 0.144455, 0.732084, 0.0165015, 0.376114, -0.224139, 0.109117, 0.10845, 0.22548, 0.355541, 0.466, 0.591047, 0.0420865, 0.061781, 0.221142, -0.0226697, -0.0946886, 0.59408, -0.210148, 0.763498, -0.134322, -0.421866, -0.145122, 0.438218, -0.00783932, -0.231399, 0.0323285, -0.316576, 0.341361, -0.68103, -0.0243779, -0.17985, -0.426674, -0.298801, 0.108337, -0.296674, -0.238458, 0.172014, -0.174808, -0.460359, 0.259288, -0.0853164, -0.258529, 0.098525, 0.0729553, -0.0739903, -0.378666, -0.0755463, -0.0974758, 0.710933, -0.213924, -0.446644, 0.181218, 0.241463, -0.338575, -0.474937, -0.316732, -0.115134, 0.108825, 0.0994124, -0.430661, -0.398433, 0.0478932, -0.276852, 0.188654, -0.36831, -0.2512, 0.19753, 0.603721, -0.310168, -0.136976, -0.115331, 0.280881, 2.82168, 0.273555, -0.379402, 1.37993, -0.0863799, 0.152367, -0.0149924, -0.0248687, -0.218092, 0.519853, 0.755826, -1.41476, -3.11583, -0.105513, 0.292712, -0.104742, 0.540286, 0.216031, 0.451483, 0.505994, -0.289062, -0.179354, -0.166658, -0.0761549, 0.0110979, -0.0506017, -0.141391, -0.0692775, 0.335116, -0.121217, -0.0360731, -0.165631, 0.0378446, 0.0369415, 0.185839, 0.329659, 0.223427, -0.490514, -0.566173, -0.141592, -0.012197, -0.432267, 0.211663, -0.502916, 0.249383, -0.580726, 0.329631, 0.0119444, -0.285195, -0.0966694, 0.691713, -0.168006, 0.0503492, 0.184251, -0.45653, -0.222412, 0.333396, 0.300319, 0.827396, 0.265502, 0.676945, 0.445713, -0.430502, -0.174229, 0.675719, 0.146302, 0.614689, -0.301544, -1.33061, 0.180319, -0.506471, 0.0911605, -0.418429, 0.0248688, 0.326793, -0.0338085, -0.613706, 0.496161, -0.369321, -0.362202, 0.282892, -0.223604, -0.171822, -0.121366, -0.560229, 0.191005, 0.269453, 0.342775, 0.088012, -0.397568, -0.310083, 0.00975675, -0.0031181, 0.0717479, -0.0646299, -0.209046, 0.302266, -0.0420151, -0.338204, 0.781986, 0.0558212, 0.174349, -0.184115, -0.141158, 0.0937934, 0.0356107, -0.35757, -0.0977751, -0.28578, 0.402887, 0.373895, 0.052216, 0.167267, -0.0270453, -0.361532, 0.0365815, -0.326877, -0.0859462, -0.38581, -0.440899, -0.100517, 0.155986, -0.298194, 0.108126, -0.438261, -0.0530153, 0.158798, -0.1023, 0.0648254, 0.087181, -0.0706768, 0.00859708, -0.130195, -0.269226, 0.0466244, -0.262494, 0.174568, -0.149424, 0.233708, 0.591514, 1.44831, 0.299433, 0.103949, -0.0791139, 0.502467, 0.199224, 0.666286, -0.280847, -0.240466, 0.336204, -0.233024, -0.396938, -2.66225, -0.271112, 0.0651703, -0.0341975, 0.816078, -0.114346, 0.218793, 0.168812, 0.0030488, 0.311466, 0.272432, 0.234685, 0.131289, -0.11595, 0.043426, 0.0842895, -0.0603468, 0.143523, -0.00369902, 0.314739, 0.260985, 0.105407, 0.159863, 0.669864, -0.70151, 0.0997946, -0.385432, 0.355837, -0.0665813, -0.0892183, 0.26739, -0.150991, 0.200124, -0.0917435, 0.261095, 0.150245, -0.332318, -0.537708, 0.263268, 0.106477, -0.101342, -0.108245, 0.327684, -0.557163};
const real_t DreicerNeuralNetwork::W1[20*8]  = {-0.0300745, -0.0492966, -0.0259351, -0.197778, 0.00746702, -0.292482, 0.486479, -0.211772, -0.00573302, 0.108765, -0.079638, -0.0446178, 0.0284712, 0.369877, -0.573668, -0.276634, -0.00714788, 0.0555219, -0.0839504, 0.166242, 0.0119449, 0.0110446, -0.418285, -0.405613, 0.0445892, -0.006038, 0.0154224, 0.187488, -0.0184206, 0.310383, 0.191048, 0.412188, -0.0759706, 0.133872, 0.657604, -0.0339612, 0.00728639, 0.634632, -0.0816593, 0.126824, -0.128342, 0.0882125, -0.12302, 0.0638418, -0.220096, 0.229134, 0.021917, -0.157344, 0.0269302, -0.147517, -0.122726, -0.49509, 0.0044491, -0.454955, 0.530144, 0.446365, -0.229462, 0.338899, 0.740651, -0.199537, 0.0154031, 0.334411, -0.0424001, 0.411041, -0.102616, 0.0592563, 0.23225, 0.462355, -0.0017818, 0.363935, -0.018741, -0.692086, -0.114089, -0.233469, -0.073154, 0.204925, 0.0608162, -0.244264, -0.174056, 0.153958, 0.158092, -0.112047, -0.0659573, 0.0238271, -0.0157809, 0.549709, -0.356053, 0.0474924, 0.0903593, -0.000213915, -0.0953753, -0.085121, 0.123612, -0.340824, 0.197758, -0.396231, 0.22071, 0.0827269, 0.148028, -0.094785, -0.0225893, -0.683586, -0.145001, 0.507944, -0.239853, 0.128895, 0.0402638, 0.241635, 0.0516254, 0.356247, 0.41492, -0.378476, -0.123678, -0.230669, 0.389936, 0.221666, -0.0983403, -0.251997, 0.175946, -0.407928, 0.537919, 0.263979, -0.00487386, -0.294119, -0.0167992, 0.0143781, 0.213622, -0.188774, 0.522746, 0.0748574, -0.598024, -0.294086, 0.00270149, -0.507866, -0.0245604, 0.165961, -0.412066, 0.137671, -0.0102158, 0.570288, 0.0135675, -0.318062, 0.030148, 0.354269, 0.594048, -0.140674, 0.102923, -0.0371824, -0.0358325, -0.362427, 0.0683319, -0.101085, -0.0101526, 0.016023, -0.0251902, 0.038211, -0.0152584, 0.211074, 0.664183, 0.526614};

/**
 * Constructor.
 */
DreicerNeuralNetwork::DreicerNeuralNetwork(RunawayFluid *rf)
    : REFluid(rf) {}

/**
 * Returns 'true' if the neural network can be applied to the
 * given temperature. The network is only trained on a certain
 * temperature range and can give ridiculous results if evaluated
 * outside of the range. While the runaway rate concept is not
 * sensible outside of this range, the Connor-Hastie formula
 * may give less nonsensical values outside the range of
 * validity of this neural network.
 *
 * The network is applicable for T in [1,20e3] eV.
 *
 * T: Electron temperature.
 */
bool DreicerNeuralNetwork::IsApplicable(const real_t T) {
    return (T <= 20e3 && T >= 1);
}
/**
 * Evaluate the runaway rate.
 *
 * ir:   Radial index for which to evaluate the runaway rate.
 * E:    Electric field strength.
 * ntot: Total electron density.
 * T:    Electron temperature.
 */
real_t DreicerNeuralNetwork::RunawayRate(
    const len_t ir, const real_t E, const real_t ntot, const real_t T
) {
    IonHandler *ions = REFluid->GetIonHandler();

    real_t nfree    = ions->GetFreeElectronDensityFromQuasiNeutrality(ir);
    real_t ED       = REFluid->GetDreicerElectricField(ir);
    real_t tauEE    = REFluid->GetElectronCollisionTimeThermal(ir);

    real_t Zeff     = ions->GetZeff(ir);
    real_t Zeff0    = ions->evaluateZeff0(ir);
    real_t Z0Z      = ions->evaluateZ0Z(ir);
    real_t Z0_Z     = ions->evaluateZ0_Z(ir);
    real_t logNfree = log(nfree);
    real_t free_tot = nfree / ntot;
    real_t logTheta = log(T/Constants::mc2inEV);

    real_t rr = RunawayRate_derived_params(
        fabs(E)/ED, logTheta, Zeff, Zeff0, Z0Z, Z0_Z,
        logNfree, free_tot
    );

    return 4.0/(3.0*sqrt(M_PI))*(nfree/tauEE) * rr;
}

/**
 * Inner function for evaluating neural network, taking a number of
 * "derived" parameters as input.
 *
 * EED:        Electric field, normalized to the Dreicer field.
 * logTheta:   (Natural) logarithm of electron temperature, divided by mc^2.
 * Zeff:       Effective plasma charge (sum_i( n_i*Z_i0^2 ) / nfree)
 * Zeff0:      sum_i( n_i*(Z_i^2 - Z_i0^2) ) / ntot
 * ZZ0:        sum_i( n_i*Z_i*Z_i0 ) / ntot
 * Z0_Z:       sum_i( n_i*Z_i/Z_i0 ) / ntot
 * logNfree:   (Natural) logarithm of free electron density.
 * nfree_ntot: Free electron density divided by total electron density.
 */
real_t DreicerNeuralNetwork::RunawayRate_derived_params(
    const real_t EED, const real_t logTheta, const real_t Zeff,
    const real_t Zeff0, const real_t ZZ0, const real_t Z0_Z,
    const real_t logNfree, const real_t nfree_ntot
) {
    real_t input[8] = {Zeff, Zeff0, Z0_Z, ZZ0, logNfree, nfree_ntot, EED, logTheta};
    real_t x1[20], x2[20], logGamma;

    // Normalize input
    for (len_t i = 0; i < 8; i++)
        input[i] = (input[i] - input_mean[i]) / input_std[i];

    nn_layer(20, 8,  W1, input, b1, x1);
    nn_layer(20, 20, W2, x1, b2, x2);
    nn_layer(20, 20, W3, x2, b3, x1);
    nn_layer(20, 20, W4, x1, b4, x2);
    nn_layer(1,  20, W5, x2, b5, &logGamma, false);

    // Denormalize output
    return exp(logGamma*output_std[0] + output_mean[0]);
}

/**
 * Evaluate one layer of the neural network. The network is
 * evaluated according to
 *
 *   out[i] = sum_j tanh(W_ij*x_j + b_i).
 *
 * nrows: Number of rows in weight matrix (elements in bias vector).
 * ncols: Number of columns in weight matrix.
 * W:     Weight matrix.
 * x:     Input vector.
 * b:     Bias vector.
 * out:   Output vector (must NOT be the same as 'x').
 * a..n:  If 'true', applies the transfer function 'tanh()' to the
 *        transformed input. This should not be done in the final
 *        stage of the neural network.
 */
void DreicerNeuralNetwork::nn_layer(
    const len_t nrows, const len_t ncols,
    const real_t *W, const real_t *x,
    const real_t *b, real_t *out,
    bool applyTransferFunction
) {
    for (len_t i = 0; i < nrows; i++) {
        real_t v = 0;
        for (len_t j = 0; j < ncols; j++)
            v += W[i*ncols + j] * x[j];

        if (applyTransferFunction)
            out[i] = tanh(v + b[i]);
        else
            out[i] = v+b[i];
    }
}

