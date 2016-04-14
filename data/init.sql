-- create the user and database
CREATE USER 'legoberry'@'localhost' IDENTIFIED BY 'legoberry123456';
CREATE DATABASE legoberry;
ALTER DATABASE legoberry CHARACTER SET utf8 COLLATE utf8_general_ci;
GRANT ALL PRIVILEGES ON legoberry.* TO 'legoberry'@'localhost';

-- create the tables
USE legoberry;

CREATE TABLE readings (
	id BIGINT PRIMARY KEY AUTO_INCREMENT,
	sensorId VARCHAR(128) NOT NULL,
	value VARCHAR(128) NOT NULL,
	timestamp VARCHAR(128) NOT NULL
);
