import React, { useState } from 'react';
import { Box, Switch, FormControlLabel, TextField, Collapse, Card, CardContent, CardHeader } from '@mui/material';

const SportsSettings = () => {
    const [nflEnabled, setNflEnabled] = useState(false);
    const [nbaEnabled, setNbaEnabled] = useState(false);

    const handleToggle = (setter) => (event) => {
        setter(event.target.checked);
    };

    return (
        <Card variant="outlined" sx={{ mt: 2 }}>
            <CardHeader title="Sports" />
            <CardContent>
                <FormControlLabel
                    control={<Switch checked={nflEnabled} onChange={handleToggle(setNflEnabled)} />}
                    label="NFL"
                />
                <Collapse in={nflEnabled}>
                    <Box pl={4}>
                        <TextField label="Hours Before Now" type="number" />
                        <TextField label="Hours After Now" type="number" />
                    </Box>
                </Collapse>
                <FormControlLabel
                    control={<Switch checked={nbaEnabled} onChange={handleToggle(setNbaEnabled)} />}
                    label="NBA"
                />
                <Collapse in={nbaEnabled}>
                    <Box pl={4}>
                        <TextField label="Hours Before Now" type="number" />
                        <TextField label="Hours After Now" type="number" />
                    </Box>

                </Collapse>
            </CardContent>
        </Card>
    );
};

export default SportsSettings;
