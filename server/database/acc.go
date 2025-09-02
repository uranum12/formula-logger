package database

import (
	"github.com/jmoiron/sqlx"

	"formula-logger/server/model"
)

const sqlInitAcc = `
	CREATE TABLE IF NOT EXISTS acc_data (
		id INTEGER PRIMARY KEY,
		time INTEGER,
		usec INTEGER,
		accel_x INTEGER,
		accel_y INTEGER,
		accel_z INTEGER,
		gyro_x INTEGER,
		gyro_y INTEGER,
		gyro_z INTEGER,
		mag_x INTEGER,
		mag_y INTEGER,
		mag_z INTEGER,
		euler_heading INTEGER,
		euler_roll INTEGER,
		euler_pitch INTEGER,
		quaternion_w INTEGER,
		quaternion_x INTEGER,
		quaternion_y INTEGER,
		quaternion_z INTEGER,
		linear_accel_x INTEGER,
		linear_accel_y INTEGER,
		linear_accel_z INTEGER,
		gravity_x INTEGER,
		gravity_y INTEGER,
		gravity_z INTEGER,
		status_sys INTEGER,
		status_gyro INTEGER,
		status_accel INTEGER,
		status_mag INTEGER
	);
`

const sqlAddAcc = `
	INSERT INTO acc_data (
		time, usec,
		accel_x, accel_y, accel_z,
		gyro_x, gyro_y, gyro_z,
		mag_x, mag_y, mag_z,
		euler_heading, euler_roll, euler_pitch,
		quaternion_w, quaternion_x, quaternion_y, quaternion_z,
		linear_accel_x, linear_accel_y, linear_accel_z,
		gravity_x, gravity_y, gravity_z,
		status_sys, status_gyro, status_accel, status_mag
	) VALUES (
		:time, :usec,
		:accel_x, :accel_y, :accel_z,
		:gyro_x, :gyro_y, :gyro_z,
		:mag_x, :mag_y, :mag_z,
		:euler_heading, :euler_roll, :euler_pitch,
		:quaternion_w, :quaternion_x, :quaternion_y, :quaternion_z,
		:linear_accel_x, :linear_accel_y, :linear_accel_z,
		:gravity_x, :gravity_y, :gravity_z,
		:status_sys, :status_gyro, :status_accel, :status_mag
	)
`

const sqlSelectTimeAcc = `
	SELECT time, usec
	FROM acc_data
	ORDER BY time ASC
`

const sqlSelectDataAcc = `
	SELECT
		usec,
		accel_x, accel_y, accel_z,
		gyro_x, gyro_y, gyro_z,
		mag_x, mag_y, mag_z,
		euler_heading, euler_roll, euler_pitch,
		quaternion_w, quaternion_x, quaternion_y, quaternion_z,
		linear_accel_x, linear_accel_y, linear_accel_z,
		gravity_x, gravity_y, gravity_z,
		status_sys, status_gyro, status_accel, status_mag
	FROM acc_data
	WHERE time BETWEEN ? AND ?
	ORDER BY time ASC
`

func InitAcc(db *sqlx.DB) {
	initTable(db, sqlInitAcc)
}

func AddAcc(tx *sqlx.Tx, data []model.AccDB) {
	addData(tx, sqlAddAcc, data)
}

func GetAccTime(db *sqlx.DB) []model.Time {
	return getTime(db, sqlSelectTimeAcc)
}

func GetAcc(db *sqlx.DB, timeMin, timeMax int64) []model.Acc {
	return getData[model.Acc](db, sqlSelectDataAcc, timeMin, timeMax)
}
