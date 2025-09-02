package model

type Acc struct {
	Usec         uint64 `db:"usec" json:"usec"`
	AccelX       int16  `db:"accel_x" json:"ax"`
	AccelY       int16  `db:"accel_y" json:"ay"`
	AccelZ       int16  `db:"accel_z" json:"az"`
	GyroX        int16  `db:"gyro_x" json:"gx"`
	GyroY        int16  `db:"gyro_y" json:"gy"`
	GyroZ        int16  `db:"gyro_z" json:"gz"`
	MagX         int16  `db:"mag_x" json:"mx"`
	MagY         int16  `db:"mag_y" json:"my"`
	MagZ         int16  `db:"mag_z" json:"mz"`
	EulerHeading int16  `db:"euler_heading" json:"h"`
	EulerRoll    int16  `db:"euler_roll" json:"r"`
	EulerPitch   int16  `db:"euler_pitch" json:"p"`
	QuaternionW  int16  `db:"quaternion_w" json:"qw"`
	QuaternionX  int16  `db:"quaternion_x" json:"qx"`
	QuaternionY  int16  `db:"quaternion_y" json:"qy"`
	QuaternionZ  int16  `db:"quaternion_z" json:"qz"`
	LinearAccelX int16  `db:"linear_accel_x" json:"lx"`
	LinearAccelY int16  `db:"linear_accel_y" json:"ly"`
	LinearAccelZ int16  `db:"linear_accel_z" json:"lz"`
	GravityX     int16  `db:"gravity_x" json:"x"`
	GravityY     int16  `db:"gravity_y" json:"y"`
	GravityZ     int16  `db:"gravity_z" json:"z"`
	StatusSys    int16  `db:"status_sys" json:"ss"`
	StatusGyro   int16  `db:"status_gyro" json:"sg"`
	StatusAccel  int16  `db:"status_accel" json:"sa"`
	StatusMag    int16  `db:"status_mag" json:"sm"`
}

type AccDB struct {
	Acc
	Time int64 `db:"time"`
}
