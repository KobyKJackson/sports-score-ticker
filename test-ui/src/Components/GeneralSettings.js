import React, { useState } from 'react';
import { Box, Switch, FormControlLabel, TextField, Collapse, Card, CardContent, CardHeader } from '@mui/material';


const GeneralSettings = () => {
    const [scheduledTimeOffEnabled, setScheduledTimeOffEnabled] = useState(false);
    const [updateRate, setUpdateRate] = useState(60); // Defaulting to 60 seconds as an example


    const handleToggleScheduledTimeOff = (event) => {
        setScheduledTimeOffEnabled(event.target.checked);
    };

    return (
        <Card variant="outlined" sx={{ mt: 2 }}>
            <CardHeader title="General Settings" />
            <CardContent>
                <FormControlLabel
                    control={<Switch checked={scheduledTimeOffEnabled} onChange={handleToggleScheduledTimeOff} />}
                    label="Scheduled Time Off"
                />
                <Collapse in={scheduledTimeOffEnabled}>
                    <Box pl={4} pt={2}>
                        <TextField
                            label="Off Time"
                            type="time"
                            InputLabelProps={{
                                shrink: true,
                            }}
                            inputProps={{
                                step: 300, // 5 minutes
                            }}
                            sx={{ mr: 2 }}
                        />
                        <TextField
                            label="On Time"
                            type="time"
                            InputLabelProps={{
                                shrink: true,
                            }}
                            inputProps={{
                                step: 300, // 5 minutes
                            }}
                        />
                    </Box>
                </Collapse>
                <TextField
                    label="Update Rate (Seconds)"
                    type="number"
                    value={updateRate}
                    onChange={(e) => setUpdateRate(e.target.value)}
                    InputProps={{
                        inputProps: {
                            min: 1, max: 120
                        }
                    }}
                    sx={{ mt: 2, display: 'block' }} // Display block to ensure it doesn't inline with other content
                />
            </CardContent>
        </Card>
    );
};

export default GeneralSettings;
