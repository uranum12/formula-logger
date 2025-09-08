package models

type AccData struct {
	DataTime
	AccelX       int16 `json:"ax"`
	AccelY       int16 `json:"ay"`
	AccelZ       int16 `json:"az"`
	GyroX        int16 `json:"gx"`
	GyroY        int16 `json:"gy"`
	GyroZ        int16 `json:"gz"`
	MagX         int16 `json:"mx"`
	MagY         int16 `json:"my"`
	MagZ         int16 `json:"mz"`
	EulerHeading int16 `json:"h"`
	EulerRoll    int16 `json:"r"`
	EulerPitch   int16 `json:"p"`
	QuaternionW  int16 `json:"qw"`
	QuaternionX  int16 `json:"qx"`
	QuaternionY  int16 `json:"qy"`
	QuaternionZ  int16 `json:"qz"`
	LinearAccelX int16 `json:"lx"`
	LinearAccelY int16 `json:"ly"`
	LinearAccelZ int16 `json:"lz"`
	GravityX     int16 `json:"x"`
	GravityY     int16 `json:"y"`
	GravityZ     int16 `json:"z"`
	StatusSys    int16 `json:"ss"`
	StatusGyro   int16 `json:"sg"`
	StatusAccel  int16 `json:"sa"`
	StatusMag    int16 `json:"sm"`
}

type AccDB struct {
	DBTime
	AccelX       float64 `json:"accel_x"`
	AccelY       float64 `json:"accel_y"`
	AccelZ       float64 `json:"accel_z"`
	GyroX        float64 `json:"gyro_x"`
	GyroY        float64 `json:"gyro_y"`
	GyroZ        float64 `json:"gyro_z"`
	MagX         float64 `json:"mag_x"`
	MagY         float64 `json:"mag_y"`
	MagZ         float64 `json:"mag_z"`
	EulerHeading float64 `json:"euler_heading"`
	EulerRoll    float64 `json:"euler_roll"`
	EulerPitch   float64 `json:"euler_pitch"`
	QuaternionW  float64 `json:"quaternion_w"`
	QuaternionX  float64 `json:"quaternion_x"`
	QuaternionY  float64 `json:"quaternion_y"`
	QuaternionZ  float64 `json:"quaternion_z"`
	LinearAccelX float64 `json:"linear_accel_x"`
	LinearAccelY float64 `json:"linear_accel_y"`
	LinearAccelZ float64 `json:"linear_accel_z"`
	GravityX     float64 `json:"gravity_x"`
	GravityY     float64 `json:"gravity_y"`
	GravityZ     float64 `json:"gravity_z"`
	StatusSys    int16   `json:"status_sys"`
	StatusGyro   int16   `json:"status_gyro"`
	StatusAccel  int16   `json:"status_accel"`
	StatusMag    int16   `json:"status_mag"`
}

func MapAccData(d AccData, t DBTime) AccDB {
	return AccDB{
		DBTime:       t,
		AccelX:       float64(d.AccelX) / 100.0,
		AccelY:       float64(d.AccelY) / 100.0,
		AccelZ:       float64(d.AccelZ) / 100.0,
		GyroX:        float64(d.GyroX) / 16.0,
		GyroY:        float64(d.GyroY) / 16.0,
		GyroZ:        float64(d.GyroZ) / 16.0,
		MagX:         float64(d.MagX) / 16.0,
		MagY:         float64(d.MagY) / 16.0,
		MagZ:         float64(d.MagZ) / 16.0,
		EulerHeading: float64(d.EulerHeading) / 16.0,
		EulerRoll:    float64(d.EulerRoll) / 16.0,
		EulerPitch:   float64(d.EulerPitch) / 16.0,
		QuaternionW:  float64(d.QuaternionW) / 16384.0,
		QuaternionX:  float64(d.QuaternionX) / 16384.0,
		QuaternionY:  float64(d.QuaternionY) / 16384.0,
		QuaternionZ:  float64(d.QuaternionZ) / 16384.0,
		LinearAccelX: float64(d.LinearAccelX) / 100.0,
		LinearAccelY: float64(d.LinearAccelY) / 100.0,
		LinearAccelZ: float64(d.LinearAccelZ) / 100.0,
		GravityX:     float64(d.GravityX) / 100.0,
		GravityY:     float64(d.GravityY) / 100.0,
		GravityZ:     float64(d.GravityZ) / 100.0,
		StatusSys:    d.StatusSys,
		StatusGyro:   d.StatusGyro,
		StatusAccel:  d.StatusAccel,
		StatusMag:    d.StatusMag,
	}
}
