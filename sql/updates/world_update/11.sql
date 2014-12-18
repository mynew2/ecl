TRUNCATE `game_graveyard_zone`;
INSERT INTO `command` (`name`, `security`, `help`) VALUES
('goname',1,'Syntax: .goname [$charactername]\r\n\r\nTeleport to the given character. Either specify the character name or click on the character\'s portrait, e.g. when you are in a group. Character can be offline.'),
('namego',1,'Syntax: .namego [$charactername]\r\n\r\nTeleport the given character to you. Character can be offline.');

INSERT INTO disables VALUES (7, 650, 0, '', '', 'Mmaps - Trial of the Champion');
INSERT INTO disables VALUES (7, 649, 0, '', '', 'Mmaps - Trial of the Crusader');
INSERT INTO disables VALUES (7, 603, 0, '', '', 'Mmaps - Ulduar');
