/*
Navicat MySQL Data Transfer

Source Server         : JBR
Source Server Version : 50509
Source Host           : localhost:3306
Source Database       : characters

Target Server Type    : MYSQL
Target Server Version : 50509
File Encoding         : 65001

Date: 2015-07-09 23:34:35
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `ab_character_bot`
-- ----------------------------
DROP TABLE IF EXISTS `ab_character_bot`;
CREATE TABLE `ab_character_bot` (
  `guid` int(10) NOT NULL DEFAULT '0',
  `sequ` tinyint(4) NOT NULL DEFAULT '0',
  `entry` int(11) DEFAULT NULL,
  `phaseMask` int(11) DEFAULT NULL,
  `x` float DEFAULT NULL,
  `y` float DEFAULT NULL,
  `z` float DEFAULT NULL,
  `ang` float DEFAULT NULL,
  `sdata` text,
  PRIMARY KEY (`guid`,`sequ`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of ab_character_bot
-- ----------------------------
