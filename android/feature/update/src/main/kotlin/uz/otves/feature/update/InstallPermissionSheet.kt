package uz.otves.feature.update

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.SheetValue
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.rememberBottomSheetState
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import uz.otves.core.designsystem.OtvesDimens
import uz.otves.core.designsystem.OtvesTheme
import uz.otves.core.designsystem.R as DesignR

/** Says why Android will ask to let the app install packages, and shows the switch to look for. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
internal fun InstallPermissionSheet(
    onOpenSettings: () -> Unit,
    onDismiss: () -> Unit,
) {
    val colors = OtvesTheme.colors
    ModalBottomSheet(
        onDismissRequest = onDismiss,
        // The whole sheet at once: half of it would hide the button.
        sheetState = rememberBottomSheetState(SheetValue.Hidden, setOf(SheetValue.Hidden, SheetValue.Expanded)),
        containerColor = colors.surface,
        shape = RoundedCornerShape(topStart = 24.dp, topEnd = 24.dp),
    ) {
        Column(modifier = Modifier.padding(start = 20.dp, end = 20.dp, bottom = 20.dp)) {
            Text(
                text = stringResource(R.string.update_permission_title),
                style = MaterialTheme.typography.titleLarge,
                fontWeight = FontWeight.SemiBold,
            )
            Spacer(Modifier.height(8.dp))
            Text(
                text = stringResource(R.string.update_permission_body),
                style = MaterialTheme.typography.bodyLarge,
                color = colors.inkSecondary,
            )
            Spacer(Modifier.height(20.dp))
            SettingsPicture()
            Spacer(Modifier.height(20.dp))
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                Step(1, stringResource(R.string.update_permission_step_1))
                Step(2, stringResource(R.string.update_permission_step_2))
            }
            Spacer(Modifier.height(24.dp))
            Button(
                onClick = onOpenSettings,
                shape = RoundedCornerShape(14.dp),
                modifier = Modifier.fillMaxWidth().height(OtvesDimens.primaryTarget),
            ) {
                Text(stringResource(R.string.update_permission_open), style = MaterialTheme.typography.titleMedium)
            }
        }
    }
}

// Android's screen as the user will find it, with the switch already on.
@Composable
private fun SettingsPicture() {
    val colors = OtvesTheme.colors
    val shape = RoundedCornerShape(16.dp)
    val picture = stringResource(R.string.update_permission_picture)
    Column(
        modifier =
            Modifier
                .fillMaxWidth()
                .clip(shape)
                .border(1.dp, colors.border, shape)
                .clearAndSetSemantics { contentDescription = picture },
    ) {
        Row(
            modifier = Modifier.fillMaxWidth().background(colors.canvas).padding(horizontal = 16.dp, vertical = 14.dp),
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Box(
                modifier =
                    Modifier
                        .size(40.dp)
                        .clip(CircleShape)
                        .background(colors.surface)
                        .border(1.dp, colors.border, CircleShape),
                contentAlignment = Alignment.Center,
            ) {
                Image(
                    painter = painterResource(DesignR.drawable.ic_otves_mark),
                    contentDescription = null,
                    modifier = Modifier.size(28.dp),
                )
            }
            Column {
                Text(
                    text = stringResource(R.string.update_permission_app),
                    style = MaterialTheme.typography.bodyLarge,
                    fontWeight = FontWeight.SemiBold,
                )
                Text(
                    text = stringResource(R.string.update_permission_screen),
                    style = MaterialTheme.typography.bodySmall,
                    color = colors.inkSecondary,
                )
            }
        }
        HorizontalDivider(color = colors.border)
        Row(
            modifier = Modifier.fillMaxWidth().padding(16.dp),
            horizontalArrangement = Arrangement.spacedBy(16.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Text(
                text = stringResource(R.string.update_permission_switch),
                style = MaterialTheme.typography.bodyLarge,
                modifier = Modifier.weight(1f),
            )
            Switch(
                checked = true,
                onCheckedChange = null,
                colors = SwitchDefaults.colors(checkedTrackColor = colors.ink, checkedThumbColor = colors.surface),
            )
        }
    }
}

@Composable
private fun Step(
    number: Int,
    text: String,
) {
    val colors = OtvesTheme.colors
    Row(horizontalArrangement = Arrangement.spacedBy(12.dp), verticalAlignment = Alignment.CenterVertically) {
        Box(
            modifier = Modifier.size(24.dp).border(1.5.dp, colors.ink, CircleShape),
            contentAlignment = Alignment.Center,
        ) {
            Text(number.toString(), style = MaterialTheme.typography.labelMedium, fontWeight = FontWeight.SemiBold)
        }
        Text(text, style = MaterialTheme.typography.bodyLarge)
    }
}
